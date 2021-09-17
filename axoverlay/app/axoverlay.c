/**
 * Copyright (C) 2020, Axis Communications AB, Lund, Sweden
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * - axoverlay -
 *
 * This application demonstrates how the use the API axoverlay, by drawing
 * plain boxes using 4-bit palette color format and text overlay using
 * ARGB32 color format.
 *
 * Colorspace and alignment:
 * 1-bit palette (AXOVERLAY_COLORSPACE_1BIT_PALETTE): 32-byte alignment
 * 4-bit palette (AXOVERLAY_COLORSPACE_4BIT_PALETTE): 16-byte alignment
 * ARGB32 (AXOVERLAY_COLORSPACE_ARGB32): 16-byte alignment
 *
 */

#include <errno.h>
#include <glib.h>
#include <cairo/cairo.h>
#include <axoverlay.h>
#include <syslog.h>

#define PALETTE_VALUE_RANGE 255.0

static GMainLoop *main_loop = NULL;
static gint animation_timer = -1;
static gint overlay_id      = -1;
static gint overlay_id_text = -1;
static gint counter = 10;
static gint top_color = 1;
static gint bottom_color = 3;

/***** Drawing functions *****************************************************/

/**
 * brief Converts palette color index to cairo color value.
 *
 * This function converts the palette index, which has been initialized by
 * function axoverlay_set_palette_color to a value that can be used by
 * function cairo_set_source_rgba.
 *
 * param color_index Index in the palette setup.
 *
 * return color value.
 */
static gdouble
index2cairo(gint color_index)
{
  return ((color_index << 4) + color_index) / PALETTE_VALUE_RANGE;
}

/**
 * brief Draw a rectangle using palette.
 *
 * This function draws a rectangle with lines from coordinates
 * left, top, right and bottom with a palette color index and
 * line width.
 *
 * param context Cairo rendering context.
 * param left Left coordinate (x1).
 * param top Top coordinate (y1).
 * param right Right coordinate (x2).
 * param bottom Bottom coordinate (y2).
 * param color_index Palette color index.
 * param line_width Rectange line width.
 */
static void
draw_rectangle(cairo_t *context, gint left, gint top,
               gint right, gint bottom,
               gint color_index, gint line_width)
{
  gdouble val = 0;

  val = index2cairo(color_index);
  cairo_set_source_rgba(context, val, val, val, val);
  cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
  cairo_set_line_width(context, line_width);
  cairo_rectangle(context, left, top, right - left, bottom - top);
  cairo_stroke(context);
}

/**
 * brief Draw a text using cairo.
 *
 * This function draws a text with a specified middle position,
 * which will be adjusted depending on the text length.
 *
 * param context Cairo rendering context.
 * param pos_x Center position coordinate (x).
 * param pos_y Center position coordinate (y).
 */
static void
draw_text(cairo_t *context, gint pos_x, gint pos_y)
{
  cairo_text_extents_t te;
  cairo_text_extents_t te_length;
  gchar *str = NULL;
  gchar *str_length = NULL;

  //  Show text in black
  cairo_set_source_rgb(context, 0, 0, 0);
  cairo_select_font_face(context, "serif", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(context, 32.0);

  // Position the text at a fix centered position
  str_length = g_strdup_printf("Countdown  ");
  cairo_text_extents(context, str_length, &te_length);
  cairo_move_to(context, pos_x - te_length.width / 2, pos_y);
  g_free(str_length);

  // Add the counter number to the shown text
  str = g_strdup_printf("Countdown %i", counter);
  cairo_text_extents(context, str, &te);
  cairo_show_text(context, str);
  g_free(str);
}

/**
 * brief Setup an overlay_data struct.
 *
 * This function initialize and setup an overlay_data
 * struct with default values.
 *
 * param data The overlay data struct to initialize.
 */
static void
setup_axoverlay_data(struct axoverlay_overlay_data *data)
{
  axoverlay_init_overlay_data(data);
  data->postype = AXOVERLAY_CUSTOM_NORMALIZED;
  data->anchor_point = AXOVERLAY_ANCHOR_CENTER;
  data->x = 0.0;
  data->y = 0.0;
  data->scale_to_stream = FALSE;
}

/**
 * brief Setup palette color table.
 *
 * This function initialize and setup an palette index
 * representing ARGB values.
 *
 * param color_index Palette color index.
 * param r R (red) value.
 * param g G (green) value.
 * param b B (blue) value.
 * param a A (alpha) value.
 *
 * return result as boolean
 */
static gboolean
setup_palette_color(gint index, gint r, gint g, gint b, gint a)
{
  GError *error = NULL;
  struct axoverlay_palette_color color;

  color.red = r;
  color.green = g;
  color.blue = b;
  color.alpha = a;
  color.pixelate = FALSE;
  axoverlay_set_palette_color(index, &color, &error);
  if (error != NULL) {
    g_error_free(error);
    return FALSE;
  }

  return TRUE;
}

/***** Callback functions ****************************************************/

/**
 * brief A callback function called when an overlay needs adjustments.
 *
 * This function is called to let developers make adjustments to
 * the size and position of their overlays for each stream. This callback
 * function is called prior to rendering every time when an overlay
 * is rendered on a stream, which is useful if the resolution has been
 * updated or rotation has changed.
 *
 * param id Overlay id.
 * param stream Information about the rendered stream.
 * param postype The position type.
 * param overlay_x The x coordinate of the overlay.
 * param overlay_y The y coordinate of the overlay.
 * param overlay_width Overlay width.
 * param overlay_height Overlay height.
 * param user_data Optional user data associated with this overlay.
 */
static void
adjustment_cb(gint id, struct axoverlay_stream_data *stream,
              enum axoverlay_position_type *postype,
              gfloat *overlay_x, gfloat *overlay_y,
              gint *overlay_width, gint *overlay_height,
              gpointer user_data)
{
  syslog(LOG_INFO, "Adjust callback for overlay: %i x %i", *overlay_width, *overlay_height);
  syslog(LOG_INFO, "Adjust callback for stream: %i x %i", stream->width, stream->height);

  *overlay_width = stream->width;
  *overlay_height = stream->height;
}

/**
 * brief A callback function called when an overlay needs to be drawn.
 *
 * This function is called whenever the system redraws an overlay. This can
 * happen in two cases, axoverlay_redraw() is called or a new stream is
 * started.
 *
 * param rendering_context A pointer to the rendering context.
 * param id Overlay id.
 * param stream Information about the rendered stream.
 * param postype The position type.
 * param overlay_x The x coordinate of the overlay.
 * param overlay_y The y coordinate of the overlay.
 * param overlay_width Overlay width.
 * param overlay_height Overlay height.
 * param user_data Optional user data associated with this overlay.
 */
static void
render_overlay_cb(gpointer rendering_context, gint id,
                  struct axoverlay_stream_data *stream,
                  enum axoverlay_position_type postype, gfloat overlay_x,
                  gfloat overlay_y, gint overlay_width, gint overlay_height,
                  gpointer user_data)
{
  gdouble val = FALSE;

  syslog(LOG_INFO, "Render callback for camera: %i", stream->camera);
  syslog(LOG_INFO, "Render callback for overlay: %i x %i", overlay_width, overlay_height);
  syslog(LOG_INFO, "Render callback for stream: %i x %i", stream->width, stream->height);

  if (id == overlay_id) {
    //  Clear background by drawing a "filled" rectangle
    val = index2cairo(0);
    cairo_set_source_rgba(rendering_context, val, val, val, val);
    cairo_set_operator(rendering_context, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(rendering_context, 0, 0, stream->width, stream->height);
    cairo_fill(rendering_context);

    //  Draw a top rectangle in toggling color
    draw_rectangle(rendering_context, 0, 0, stream->width,
                   stream->height / 4, top_color, 9.6);

    //  Draw a bottom rectangle in toggling color
    draw_rectangle(rendering_context, 0, stream->height * 3 / 4, stream->width,
                   stream->height, bottom_color, 2.0);
  } else if (id == overlay_id_text) {
    //  Show text in black
    draw_text(rendering_context, stream->width / 2, stream->height / 2);
  } else {
    syslog(LOG_INFO, "Unknown overlay id!");
  }
}

/**
 * brief Callback function which is called when animation timer has elapsed.
 *
 * This function is called when the animation timer has elapsed, which will
 * update the counter, colors and also trigger a redraw of the overlay.
 *
 * param user_data Optional callback user data.
 */
static gboolean
update_overlay_cb(gpointer user_data)
{
  GError *error = NULL;

  // Countdown
  counter = counter < 1 ? 10 : counter - 1;

  if (counter == 0) {
    // A small color surprise
    top_color = top_color > 2 ? 1 : top_color + 1;
    bottom_color = bottom_color > 2 ? 1 : bottom_color + 1;
  }

  // Request a redraw of the overlay
  axoverlay_redraw(&error);
  if (error != NULL) {
    /*
     * If redraw fails then it is likely due to that overlayd has
     * crashed. Don't exit instead wait for overlayd to restart and
     * for axoverlay to restore the connection.
     */
    syslog(LOG_ERR, "Failed to redraw overlay (%d): %s", error->code, error->message);
    g_error_free(error);
  }

  return G_SOURCE_CONTINUE;
}

/***** Signal handler functions **********************************************/

/**
 * brief Handles the signals.
 *
 * This function handles the signals when it is time to
 * quit the main loop.
 *
 * param signal_num Signal number.
 */
static void
signal_handler(gint signal_num)
{
  switch (signal_num) {
  case SIGTERM:
  case SIGABRT:
  case SIGINT:
    g_main_loop_quit(main_loop);
    break;
  default:
    break;
  }
}

/**
 * brief Initialize the signal handler.
 *
 * This function handles the initialization of signals.
 *
 * return result as boolean.
 */
static gboolean
signal_handler_init(void)
{
  struct sigaction sa = {0};

  if (sigemptyset(&sa.sa_mask) == -1) {
    syslog(LOG_ERR, "Failed to initialize signal handler: %s", strerror(errno));
    return FALSE;
  }

  sa.sa_handler = signal_handler;

  if ((sigaction(SIGTERM, &sa, NULL) < 0) ||
      (sigaction(SIGABRT, &sa, NULL) < 0) ||
      (sigaction(SIGINT, &sa, NULL) < 0)) {
    syslog(LOG_ERR, "Failed to install signal handler: %s", strerror(errno));
    return FALSE;
  }

  return TRUE;
}

/***** Main function *********************************************************/

/**
 * brief Main function.
 *
 * This main function draws two plain boxes and one text, using the
 * API axoverlay.
 *
 * param argc Number of arguments.
 * param argv Arguments vector.
 */
int
main(int argc, char **argv)
{
  GError *error = NULL;
  GError *error_text = NULL;
  gint camera_height = 0;
  gint camera_width = 0;

  openlog(NULL, LOG_PID, LOG_USER);

  if (!signal_handler_init()) {
    syslog(LOG_ERR, "Could not set up signal handler");
    return 1;
  }

  //  Create a glib main loop
  main_loop = g_main_loop_new(NULL, FALSE);

  if(!axoverlay_is_backend_supported(AXOVERLAY_CAIRO_IMAGE_BACKEND)) {
    syslog(LOG_ERR, "AXOVERLAY_CAIRO_IMAGE_BACKEND is not supported");
    return 1;
  }

  //  Initialize the library
  struct axoverlay_settings settings;
  axoverlay_init_axoverlay_settings(&settings);
  settings.render_callback = render_overlay_cb;
  settings.adjustment_callback = adjustment_cb;
  settings.select_callback = NULL;
  settings.backend = AXOVERLAY_CAIRO_IMAGE_BACKEND;
  axoverlay_init(&settings, &error);
  if (error != NULL) {
    syslog(LOG_ERR, "Failed to initialize axoverlay: %s", error->message);
    g_error_free(error);
    return 1;
  }

  //  Setup colors
  if (!setup_palette_color(0, 0, 0, 0, 0) ||
      !setup_palette_color(1, 255, 0, 0, 255) ||
      !setup_palette_color(2, 0, 255, 0, 255) ||
      !setup_palette_color(3, 0, 0, 255, 255)) {
    syslog(LOG_ERR, "Failed to setup palette colors");
    return 1;
  }

  // Get max resolution for width and height
  camera_width = axoverlay_get_max_resolution_width(1, &error);
  g_error_free(error);
  camera_height = axoverlay_get_max_resolution_height(1, &error);
  g_error_free(error);
  syslog(LOG_INFO, "Max resolution (width x height): %i x %i", camera_width,
         camera_height);

  // Create a large overlay using Palette color space
  struct axoverlay_overlay_data data;
  setup_axoverlay_data(&data);
  data.width = camera_width;
  data.height = camera_height;
  data.colorspace = AXOVERLAY_COLORSPACE_4BIT_PALETTE;
  overlay_id = axoverlay_create_overlay(&data, NULL, &error);
  if (error != NULL) {
    syslog(LOG_ERR, "Failed to create first overlay: %s", error->message);
    g_error_free(error);
    return 1;
  }

  // Create an text overlay using ARGB32 color space
  struct axoverlay_overlay_data data_text;
  setup_axoverlay_data(&data_text);
  data_text.width = camera_width;
  data_text.height = camera_height;
  data_text.colorspace = AXOVERLAY_COLORSPACE_ARGB32;
  overlay_id_text = axoverlay_create_overlay(&data_text, NULL, &error_text);
  if (error_text != NULL) {
    syslog(LOG_ERR, "Failed to create second overlay: %s", error_text->message);
    g_error_free(error_text);
    return 1;
  }

  // Draw overlays
  axoverlay_redraw(&error);
  if (error != NULL) {
    syslog(LOG_ERR, "Failed to draw overlays: %s", error->message);
    axoverlay_destroy_overlay(overlay_id, &error);
    axoverlay_destroy_overlay(overlay_id_text, &error_text);
    axoverlay_cleanup();
    g_error_free(error);
    g_error_free(error_text);
    return 1;
  }

  // Start animation timer
  animation_timer = g_timeout_add_seconds(1, update_overlay_cb, NULL);

  // Enter main loop
  g_main_loop_run(main_loop);

  // Destroy the overlay
  axoverlay_destroy_overlay(overlay_id, &error);
    if (error != NULL) {
    syslog(LOG_ERR, "Failed to destroy first overlay: %s", error->message);
    g_error_free(error);
    return 1;
  }
  axoverlay_destroy_overlay(overlay_id_text, &error_text);
  if (error_text != NULL) {
    syslog(LOG_ERR, "Failed to destroy second overlay: %s", error_text->message);
    g_error_free(error_text);
    return 1;
  }

  // Release library resources
  axoverlay_cleanup();

  // Release the animation timer
  g_source_remove(animation_timer);

  // Release main loop
  g_main_loop_unref(main_loop);

  return 0;
}
