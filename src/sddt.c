#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xA2, 0x10, 0x8A, 0xDB, 0x8A, 0x31, 0x41, 0xF5, 0xAA, 0x5E, 0x72, 0x46, 0xF4, 0xCE, 0xE9, 0x3D }
PBL_APP_INFO(MY_UUID,
             "SDDT", "circuitben.net",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

// Hopefully these buffers are big enough for any localization.
// They're big enough for English at least.

char date_text[32];
TextLayer date_layer;

char hour_minutes_text[] = "00:00";
TextLayer hour_minutes_layer;

char seconds_text[] = ":00";
TextLayer seconds_layer;

char ampm_text[] = "AM";
TextLayer ampm_layer;

// This was the time at the last call to update_time().
// This is initialized with impossible values to ensure all layers are updated
// on the first call to update_time().
PblTm last_time = {-1, -1, -1, -1, -1, -1, -1, -1, -1};

// State of clock_is_24h_style() at the last call to update_time().
// We don't care about the initial value of this because the difference
// in last_time will cause everything to be updated.
bool last_24hr;

Layer decoration_layer;

void update_time(const PblTm *time)
{
    // Update text layers for time fields that have changed.

    // Month and day.
    if (time->tm_mon != last_time.tm_mon || time->tm_mday != last_time.tm_mday)
    {
        string_format_time(date_text, sizeof(date_text), "%A\n%B %e", time);
        text_layer_set_text(&date_layer, date_text);
    }

    bool is_24hr = clock_is_24h_style();

    // Hour and minute.
    if (time->tm_hour != last_time.tm_hour || time->tm_sec != last_time.tm_sec || is_24hr != last_24hr)
    {
        // Use snprintf instead of string_format_time() to get the right padding.
        const char *time_format;
        int hour = time->tm_hour;
        if (is_24hr)
        {
            // 24-hour style.
            // Hour is zero-padded.
            time_format = "%02d:%02d";
        } else {
            // 12-hour style.
            // Hour has no padding.
            time_format = "%d:%02d";
            if (hour == 0)
            {
                // Midnight.
                hour = 12;
            } else if (hour > 12)
            {
                hour -= 12;
            }
        }
        snprintf(hour_minutes_text, sizeof(hour_minutes_text), time_format, hour, time->tm_min);
        text_layer_set_text(&hour_minutes_layer, hour_minutes_text);
    }

    // AM/PM (blank for 24-hour style).
    if (time->tm_hour != last_time.tm_hour || is_24hr != last_24hr)
    {
        if (is_24hr)
        {
            text_layer_set_text(&ampm_layer, "");
        } else {
            string_format_time(ampm_text, sizeof(ampm_text), "%p", time);
            text_layer_set_text(&ampm_layer, ampm_text);
        }
    }

    // Seconds.
    if (time->tm_sec != last_time.tm_sec)
    {
        string_format_time(seconds_text, sizeof(seconds_text), ":%S", time);
        text_layer_set_text(&seconds_layer, seconds_text);
    }

    // Save state for next time.
    last_time = *time;
    last_24hr = is_24hr;
}

void add_text_layer(TextLayer *layer, GRect rect, GFont font, GTextAlignment align)
{
    text_layer_init(layer, rect);
    text_layer_set_text_color(layer, GColorWhite);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_font(layer, font);
    text_layer_set_text_alignment(layer, align);
    layer_add_child(&window.layer, &layer->layer);
}

void update_decoration(Layer *layer, GContext *ctx)
{
    graphics_context_set_stroke_color(ctx, GColorWhite);

    graphics_draw_line(ctx, GPoint(0, 68), GPoint(144, 68));
    graphics_draw_line(ctx, GPoint(0, 69), GPoint(144, 69));
}

void handle_init(AppContextRef ctx)
{
    window_init(&window, "SDDT");
    window_stack_push(&window, true);
    window_set_background_color(&window, GColorBlack);

    GFont font_condensed = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
    GFont font_bold = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);

    add_text_layer(&date_layer,         GRect(8, 8, 128, 52),   font_condensed, GTextAlignmentCenter);
    add_text_layer(&seconds_layer,      GRect(104, 80, 32, 22), font_condensed, GTextAlignmentRight);
    add_text_layer(&ampm_layer,         GRect(8, 80, 32, 22),   font_condensed, GTextAlignmentLeft);
    add_text_layer(&hour_minutes_layer, GRect(8, 96, 128, 50),  font_bold,      GTextAlignmentCenter);

    layer_init(&decoration_layer, window.layer.frame);
    decoration_layer.update_proc = &update_decoration;
    layer_add_child(&window.layer, &decoration_layer);

    // Update the time now.
    PblTm time;
    get_time(&time);
    update_time(&time);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    update_time(t->tick_time);
}

void pbl_main(void *params)
{
    PebbleAppHandlers handlers =
    {
        .init_handler = &handle_init,

        .tick_info =
        {
          .tick_handler = &handle_second_tick,
          .tick_units = SECOND_UNIT
        }
    };
    app_event_loop(params, &handlers);
}
