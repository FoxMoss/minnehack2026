#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <variant>
#include <vector>
#ifndef FLIGHT_SIM_STATIC
#define WSPP_USE_OPENSSL
#include "wspp.h"
#else 
#include <emscripten/emscripten.h>
void loop();
#endif
#include "raygui.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

// void handleInputs(float &x, float &y, float &z,
//                   float sensitivity = 0.5f) { // bbbbbroken!
//   // Quaternion q = QuaternionFromEuler(target_z * 45.0f * DEG2RAD, 0,
//   //                                    target_x * 45.0f * DEG2RAD);
//   // Emulate y-axis gyro (horizontal movement/roll)
//   if (IsKeyDown(KEY_RIGHT))
//     x += sensitivity;
//   if (IsKeyDown(KEY_LEFT))
//     x -= sensitivity;
//
//   // Emulate x-axis gyro (vertical movement/pitch)
//   if (IsKeyDown(KEY_UP))
//     z -= sensitivity;
//   if (IsKeyDown(KEY_DOWN))
//     z += sensitivity;
// }

class Entity {
public:
  Vector3 position;
  float rotation;
  bool active;

  Color color = WHITE;

  Entity(Vector3 pos) : position(pos), rotation(0.0f), active(true) {}

  void Update(Vector3 pos) {
    if (!active)
      return;

    rotation += 2.0f;
    if (rotation >= 360.0f)
      rotation -= 360.0f;

    // Move relative to the plane's apparent motion
    position = pos;
  }

  void draw(Model &model) {
    if (!active)
      return;
    // Draw centered at position, rotating around the Y-axis
    DrawModelEx(model, position, {0, 1, 0}, rotation, {2, 2, 2}, color);
  }

  void draw() {
    if (model == nullptr || !active)
      return;
    // Draw centered at position, rotating around the Y-axis
    DrawModelEx(*model, position, {0, 1, 0}, rotation, {1, 1, 1}, color);
  }

  void setModel(Model &model) { this->model = &model; }

private:
  Model *model = nullptr;
};

auto rng = std::default_random_engine{};
static Music intenseMusic;
static Sound gearSound;
static Camera3D camera;
static Model gasCan, truck, propeller, collectibleGear, collectibleBoard;

#ifndef FLIGHT_SIM_STATIC
static Model plane_model;
static Texture2D texture;
#else
static Model plane_model;
static Texture2D texture;
#endif

static Vector3 planePosition = {0.0f, 0.0f, 0.0f};

static float x = 0.0;
static float target_x = 0.0;

static float y = 0.0;
static float target_y = 0.0;

static float z = 0.0;
static float target_z = 0.0;

static float health = 100;
static float progression = 0;

static float cool_down_time = 0;
static float show_time = 0;
static float fire_time = 0;
static float hurt_count_down = 0;
static int hurt_object = 0;
static float manuevering_speed = 0;
static float health_dec = 0;
static float upgrade_inc = 0;
static bool magnitisim = false;

static enum GunFireState {
  FIRING_COOL_DOWN,
  FIRING_SHOWN,
  FIRING_ACTIVE
} fire_state = FIRING_COOL_DOWN;

static enum MenuState {
  MAIN_MENU,
  GAME_RUNNING,
  UPGRADE_SCREEN,
  GAME_OVER,
  PAUSE_SCREEN,
  WIN_SCREEN,
} menu_state = MAIN_MENU;


static Mesh sphere ;
static Model sky ;
static Texture2D skyboxTex ;

static std::vector<Vector3> hurt_spheres;

struct Upgrade {
  std::string name;
  enum UpgradeType {
    LESS_BARRAGES,
    FASTER_MANUEVERING,
    MORE_HEALTH,
    PROTECTION,
    INCREASE_SHOOTER_COOLDOWN,
    MAGNITISM
  } type;
  bool one_use = false;
  bool used = false;
};

static std::vector<Upgrade> upgrades;
static std::vector<Upgrade> choosable_upgrades;

static size_t frame_ticks = 0;
static size_t game_ticks = 0;

static std::vector<Entity> collectibles;

static MenuState last_state = menu_state;

#include <math.h>

std::queue<std::string> input_log;
std::optional<std::string> last_msg;

extern "C" {

void send_inputs(char* x) {
  input_log.push(x);
}

char *get_inputs() {
  if (!last_msg.has_value()) {
    return "{}";
    
  }
  char * a = (char*)malloc(last_msg->size() + 1);
  memcpy(a, last_msg->c_str(), last_msg->size() + 1);

  last_msg = {};
  return a;
}

}

int main(int argc, char *argv[]) {
  srand(time(NULL));

#ifndef FLIGHT_SIM_STATIC
  if (argc != 2) {
    printf("USAGE: %s [lobby code]\n", argv[0]);
    return 1;
  }
#endif

  /////////////////////////////////////////////////////////////////////////////
  // Window Initialization
  /////////////////////////////////////////////////////////////////////////////

  InitWindow(1024, 1024, "MinneFlight");
  SetTargetFPS(60);

  InitAudioDevice();
#ifndef FLIGHT_SIM_STATIC
  gearSound = LoadSound("../resources/gear_collect.mp3");
  intenseMusic = LoadMusicStream("../resources/intenseMusic.mp3");
#else
  gearSound = LoadSound("/gear_collect.mp3");
  intenseMusic = LoadMusicStream("/intenseMusic.mp3");
#endif
  intenseMusic.looping = true; // Enables perfect looping
  PlayMusicStream(intenseMusic);

  /////////////////////////////////////////////////////////////////////////////
  // World Initialization
  /////////////////////////////////////////////////////////////////////////////
  camera = {};
  camera.position = (Vector3){-10.0f, 1.0f, 0.0f};
  camera.target = (Vector3){0.0f, 0.0f, 0.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

#ifndef FLIGHT_SIM_STATIC
  gasCan = LoadModel("../resources/GasCan.glb");
  truck = LoadModel("../resources/truck.glb");
  propeller = LoadModel("../resources/Propeller.glb");
  collectibleGear = LoadModel("../resources/CollectibleGear.glb");
  collectibleBoard = LoadModel("../resources/CollectibleBoard.glb");
#else
  gasCan = LoadModel("/GasCan.glb");
  truck = LoadModel("/truck.glb");
  propeller = LoadModel("/Propeller.glb");
  collectibleGear = LoadModel("/CollectibleGear.glb");
  collectibleBoard = LoadModel("/CollectibleBoard.glb");
#endif

#ifndef FLIGHT_SIM_STATIC
  plane_model = LoadModel("../resources/PUSHILIN_Plane.obj");
  texture = LoadTexture("../resources/PUSHILIN_PLANE.png");
#else
  plane_model = LoadModel("/PUSHILIN_Plane.obj");
  texture = LoadTexture("/PUSHILIN_PLANE.png");
#endif
  plane_model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

  planePosition = {0.0f, 0.0f, 0.0f};

  x = 0.0;
  target_x = 0.0;

  y = 0.0;
  target_y = 0.0;

  z = 0.0;
  target_z = 0.0;

  health = 100;
  progression = 0;

  cool_down_time = 0;
  show_time = 0;
  fire_time = 0;
  hurt_count_down = 0;
  hurt_object = 0;
  manuevering_speed = 0;
  health_dec = 0;
  upgrade_inc = 0;
  magnitisim = false;

  fire_state = FIRING_COOL_DOWN;

  menu_state = MAIN_MENU;


  // Skybox code
  // sphere = GenMeshSphere(500.0f, 32, 32);
  // sky = LoadModelFromMesh(sphere);
#ifndef FLIGHT_SIM_STATIC
  // skyboxTex = LoadTexture("../resources/skybox.jpg");
#else
  // skyboxTex = LoadTexture("/skybox.jpg");
#endif
  // Texture2D skyboxPanorama = LoadTexture("resources/skybox.hdr");
  // sky.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = skyboxTex;

  /////////////////////////////////////////////////////////////////////////////
  // Thread client Initialization
  /////////////////////////////////////////////////////////////////////////////
#ifndef FLIGHT_SIM_STATIC
  wspp::ws_client c;
  c.connect("ws://foxmoss.com:9003/api/laptop_ws/" + std::string(argv[1]));
#endif


  upgrades = {
      {"Less Acid Rain", Upgrade::LESS_BARRAGES},
      {"Faster Manuevering", Upgrade::FASTER_MANUEVERING},
      {"More Health", Upgrade::MORE_HEALTH},
      {"Protection", Upgrade::PROTECTION},
      {"Seed Good Air", Upgrade::INCREASE_SHOOTER_COOLDOWN},
      {"Magnitisim", Upgrade::MAGNITISM, true},
  };

  choosable_upgrades = {};

  frame_ticks = 0;
  game_ticks = 0;


  last_state = menu_state;

#ifndef FLIGHT_SIM_STATIC
  c.on_tick([&](std::optional<wspp::message_view> msg) {
#else
  emscripten_set_main_loop(loop, 0, 1);
  UnloadSound(gearSound);
  CloseAudioDevice();
}

  void loop(){
    std::optional<std::string> msg = {};

    if (!input_log.empty()) {
      msg = input_log.front();
      input_log.pop();
    }
#endif
    UpdateMusicStream(intenseMusic);

    frame_ticks++;
    if (frame_ticks % 1000 == 0 || last_state != menu_state) {
      nlohmann::json data = {{"type", "set_game_state"},
                             {"game_state", menu_state}};
      if (menu_state == UPGRADE_SCREEN) {
        data["upgrades"] = nlohmann::json::array();

        for (auto upgrade : upgrades) {
          data["upgrades"].push_back(upgrade.type);
        }
      }

    last_msg = data.dump();
#ifndef FLIGHT_SIM_STATIC
      c.send(data.dump());
#endif
    }
    last_state = menu_state;

#ifndef FLIGHT_SIM_STATIC
    if (WindowShouldClose()) {
      c.close();
    }
#endif

    if (IsKeyDown(KEY_UP)) {
      z = -1;
    }
    if (IsKeyDown(KEY_DOWN)) {
      z = 1;
    }
    if (IsKeyDown(KEY_LEFT)) {
      x = -1;
    }
    if (IsKeyDown(KEY_RIGHT)) {
      x = 1;
    }

#ifndef FLIGHT_SIM_STATIC
    if (msg.has_value()) {
      nlohmann::json data = nlohmann::json::parse(msg->text());
#else
    if (msg.has_value()) {
      nlohmann::json data = nlohmann::json::parse(msg.value());
#endif

      printf("%s\n", data.dump().c_str());

      if (data["type"] == "gyro_update") {
        x += -(float)data["x"] / 10;
        y += (float)data["z"] / 10;
        z += (float)data["y"] / 10;
      }
      if (data["type"] == "button_down" &&
          (menu_state == MAIN_MENU || menu_state == GAME_OVER)) {
        // init game statej
        menu_state = GAME_RUNNING;
        health = 100;

        collectibles.clear();
        for (size_t i = 0; i < 20; i++) {
          collectibles.emplace_back(Vector3{
              40.0f + (i * 20.0f), (float)(std::rand() % 256) / 256 * 10 - 5,
              (float)(std::rand() % 256) / 256 * 10 - 5});
        }

        hurt_spheres.clear();

        health = 100;
        progression = 0;

        cool_down_time = 10;
        show_time = 5;
        fire_time = 5;
        hurt_count_down = cool_down_time;
        hurt_object = 1;
        manuevering_speed = 0.1;
        health_dec = 1;
        magnitisim = false;
        frame_ticks = 0;
        upgrade_inc = 2;
        game_ticks = 0;

        for (size_t i = 0; i < upgrades.size(); i++) {
          upgrades[i].used = false;
        }
        std::shuffle(upgrades.begin(), upgrades.end(), rng);

        fire_state = FIRING_COOL_DOWN;
      } else if (data["type"] == "button_down" &&
                 menu_state == UPGRADE_SCREEN) {
        std::string button_int = data["button"];
        int button_index = std::stoi(button_int);
        auto selected_upgrade = choosable_upgrades[button_index];

        printf("Upgrade selected %s\n", selected_upgrade.name.c_str());

        switch (selected_upgrade.type) {
        case Upgrade::LESS_BARRAGES:
          hurt_object -= 1;
          hurt_object = std::max(hurt_object, 1);
          break;
        case Upgrade::INCREASE_SHOOTER_COOLDOWN:
          cool_down_time++;
          break;
        case Upgrade::PROTECTION:
          health_dec /= 2;
          break;
        case Upgrade::MORE_HEALTH:
          health += 20;
          health = std::clamp(health, 0.0f, 100.0f);
          break;
        case Upgrade::FASTER_MANUEVERING:
          manuevering_speed += 0.1;
          break;
        case Upgrade::MAGNITISM:
          magnitisim = true;
          break;
        }

        menu_state = GAME_RUNNING;
        progression = 0;
        upgrade_inc /= 2;
      } else if (data["type"] == "button_down" && menu_state == PAUSE_SCREEN) {
        menu_state = GAME_RUNNING;
      } else if (data["type"] == "button_down" && menu_state == GAME_RUNNING) {
        menu_state = PAUSE_SCREEN;
      }
    }

    x = x * 0.9;
    target_x = x * 0.1 + target_x * 0.9;

    z = z * 0.9;
    target_z = z * 0.1 + target_z * 0.9;

    BeginDrawing();

    ClearBackground(Color{201, 209, 211, 255});

    BeginMode3D(camera);

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();

    Vector3 euler_rot{target_x * 90, 0, target_z * 90};
    Vector3 normalized = Vector3Normalize(euler_rot);
    float scale = Vector3Length(euler_rot);

    float up = std::sin((target_z * 90) * DEG2RAD) * 1;
    float horizontal = std::sin((target_x * 90) * DEG2RAD) * 1;

    DrawModelEx(plane_model, {0, 0, 0}, normalized, scale, {1, 1, 1}, WHITE);

    if (menu_state == GAME_RUNNING) {
      game_ticks++;

      hurt_count_down -= (float)1 / 60;
      if (hurt_count_down < 0) {
        switch (fire_state) {
        case FIRING_COOL_DOWN:
          fire_state = FIRING_SHOWN;
          hurt_count_down = show_time;

          for (size_t i = 0; i < hurt_object; i++) {
            hurt_spheres.push_back(
                {0,
                 static_cast<float>(((float)(std::rand() % 256) / 256 * 10) -
                                    2.5),
                 static_cast<float>(((float)(std::rand() % 256) / 256 * 10) -
                                    5)});
          }

          break;
        case FIRING_SHOWN:
          fire_state = FIRING_ACTIVE;
          hurt_count_down = fire_time;
          break;
        case FIRING_ACTIVE:
          fire_state = FIRING_COOL_DOWN;
          cool_down_time -= 1;
          cool_down_time = std::max(cool_down_time, 2.0f);
          show_time -= 1;
          show_time = std::max(show_time, 1.0f);
          hurt_count_down = cool_down_time;
          hurt_spheres.clear();
          hurt_object += 3;
          break;
        }
      }

      for (size_t i = 0; i < hurt_spheres.size(); i++) {
        hurt_spheres[i] -= Vector3{0, up, horizontal} * manuevering_speed;
        if (fire_state == FIRING_SHOWN && frame_ticks % 10 < 5) {
          DrawSphere(hurt_spheres[i], 1, Color{230, 41, 55, 100});
        } else if (fire_state == FIRING_ACTIVE) {
          DrawSphere(hurt_spheres[i], 1, Color{230, 41, 55, 255});
          if (Vector3Distance(Vector3Zero(), hurt_spheres[i]) < 2) {
            health -= health_dec;
          }
        }
      }

      for (size_t i = 0; i < collectibles.size(); i++) {
        collectibles[i].Update(collectibles[i].position -
                               Vector3{1.0, up, horizontal} *
                                   manuevering_speed);

        if (magnitisim) {
          float strength = (16000 - collectibles[i].position.x) / 10000000.0;

          collectibles[i].Update(Vector3Add(
              Vector3Scale(collectibles[i].position, 1 - strength),
              Vector3Scale({collectibles[i].position.x, 0, 0}, strength)));
        }
        collectibles[i].draw(collectibleGear);

        if (CheckCollisionSpheres(Vector3Zero(), 1.0f, collectibles[i].position,
                                  0.5f)) {
          collectibles[i].Update({40.0f + (i * 20.0f),
                                  (float)(std::rand() % 256) / 256 * 10 - 5,
                                  (float)(std::rand() % 256) / 256 * 10 - 5});
          PlaySound(gearSound);
          progression += upgrade_inc * 10;
        }

        if (collectibles[i].position.x < camera.position.x) {
          collectibles[i].Update({40.0f + (i * 20.0f),
                                  (float)(std::rand() % 256) / 256 * 10 - 5,
                                  (float)(std::rand() % 256) / 256 * 10 - 5});
        }
      }

    }

    EndMode3D();

    DrawFPS(10, 10);

    if (menu_state == GAME_RUNNING) {
      GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
      GuiProgressBar({1024 / 5, 1024 / 16, 1024 / 8 * 6, 30}, "Progression",
                     NULL, &progression, 0, 100);

      GuiProgressBar({1024 / 4, 1024 / 8 * 5, 1024 / 2, 20}, "Health", NULL,
                     &health, 0, 100);

      std::string firing_state_str;
      switch (fire_state) {
      case FIRING_COOL_DOWN:
        firing_state_str = "Clear skys for the next ";
        break;
      case FIRING_SHOWN:
        firing_state_str = "Acid rain forcasted in ";
        break;
      case FIRING_ACTIVE:
        firing_state_str = "Acid rain active for the next ";
        break;
      }

      DrawText(
          TextFormat("%s%.02fs", firing_state_str.c_str(), hurt_count_down),
          1024 / 16, 1024 / 8, 40, BLACK);

      DrawText(TextFormat("Time elapsed: %.02fs. Target 180.00s",
                          (float)game_ticks / 60),
               1024 / 16, 1024 / 8 * 7, 40, BLACK);

      if ((float)game_ticks / 60 > 180) {
        menu_state = WIN_SCREEN;
      }

      if (health <= 0) {
        menu_state = GAME_OVER;
      }

      if (progression >= 100) {
        menu_state = UPGRADE_SCREEN;
        std::shuffle(upgrades.begin(), upgrades.end(), rng);
        choosable_upgrades.clear();
        auto upgrade_iter = upgrades.begin();

        while (choosable_upgrades.size() != 4) {
          if (upgrade_iter->one_use && upgrade_iter->used) {
            upgrade_iter += 1;
            continue;
          }

          choosable_upgrades.push_back(*upgrade_iter.base());
          upgrade_iter += 1;
        }
      }

    } else if (menu_state == MAIN_MENU) {
      DrawTextPro(GetFontDefault(), "MinneFlight", {1024 / 2, 1024 / 5},
                  {300, 50}, std::sin(GetTime() * 2) * 5, 100, 2, BLACK);
      DrawText("Press GO to start", 1024 / 5, 1024 / 5 + 100, 40, BLACK);
    } else if (menu_state == GAME_OVER) {
      DrawTextPro(GetFontDefault(), "You died!", {1024 / 2, 1024 / 5},
                  {300, 50}, std::sin(GetTime() * 2) * 5, 100, 2, BLACK);
      DrawText("Press GO to restart", 1024 / 5, 1024 / 5 + 100, 40, BLACK);
    } else if (menu_state == PAUSE_SCREEN) {
      DrawTextPro(GetFontDefault(), "Paused", {1024 / 2, 1024 / 5}, {300, 50},
                  std::sin(GetTime() * 2) * 5, 100, 2, BLACK);
      DrawText("Press GO to continue", 1024 / 5, 1024 / 5 + 100, 40, BLACK);
    } else if (menu_state == WIN_SCREEN) {
      DrawTextPro(GetFontDefault(), "You won!", {1024 / 2, 1024 / 5}, {300, 50},
                  std::sin(GetTime() * 2) * 5, 100, 2, BLACK);
    } else if (menu_state == UPGRADE_SCREEN) {
      GuiSetStyle(DEFAULT, TEXT_SIZE, 30);
      GuiProgressBar({1024 / 5, 1024 / 16, 1024 / 8 * 6, 30}, "Progression",
                     NULL, &progression, 0, 100);

      GuiProgressBar({1024 / 4, 1024 / 8 * 5, 1024 / 2, 20}, "Health", NULL,
                     &health, 0, 100);

      DrawRectangle(0, 0, 1024, 1024, Color{255, 255, 255, 100});

      DrawTextPro(GetFontDefault(), "Upgrades!", {1024 / 2, 1024 / 7},
                  {300, 50}, std::sin(GetTime() * 2) * 5, 100, 2, BLACK);
      DrawText(TextFormat("A: %s", choosable_upgrades[0].name.c_str()),
               1024 / 16, 1024 / 3, 20, BLACK);
      DrawText(TextFormat("B: %s", choosable_upgrades[1].name.c_str()),
               1024 / 16, 1024 / 2, 20, BLACK);
      DrawText(TextFormat("X: %s", choosable_upgrades[2].name.c_str()),
               1024 / 16 * 8, 1024 / 3, 20, BLACK);
      DrawText(TextFormat("Y: %s", choosable_upgrades[3].name.c_str()),
               1024 / 16 * 8, 1024 / 2, 20, BLACK);
    }

    EndDrawing();
#ifndef FLIGHT_SIM_STATIC
  });

  c.on_close([&](auto) {
    std::cout << "ws closed\n";
    UnloadSound(gearSound);
    CloseAudioDevice();
  });

  c.run();
#else

#endif

}

