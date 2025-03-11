#include <cute.h>

void mount_content_directory_as(const char* dir)
{
  char path[256];
  snprintf(path, sizeof(path), "%s/demo_data", cf_fs_get_base_directory());
  cf_fs_mount(path, dir, false);
}

int main(int argc, char* argv[]) {
  cf_make_app(
    "Demo", 0, 0, 0, 1280, 720,
    CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]
  );
  mount_content_directory_as("/");

  CF_Sprite sprites[] = {
    cf_make_sprite("assets/sprites/backgrounds/empty.aseprite"),
    cf_make_sprite("assets/sprites/backgrounds/flowers-1.aseprite"),
    cf_make_sprite("assets/sprites/backgrounds/flowers-2.aseprite"),
    cf_make_sprite("assets/sprites/backgrounds/patch-1.aseprite"),
    cf_make_sprite("assets/sprites/backgrounds/patch-2.aseprite"),
    cf_make_sprite("assets/sprites/backgrounds/patch-3.aseprite"),
  };

  while (cf_app_is_running()) {
    cf_app_update(NULL);
    cf_clear_color(0, 1, 0, 1);

    for (int x = 0; x < 32; x++) {
      for (int y = 0; y < 32; y++) {
        cf_draw_push();
        cf_draw_translate(
          x * 32 - cf_app_get_width() / 2,
          y * 32 - cf_app_get_height() / 2
        );
        cf_draw_sprite(&sprites[(x + y) % 6]);
        cf_draw_pop();
      }
    }

    cf_app_draw_onto_screen(true);
  }

  cf_destroy_app();
  return 0;
}
