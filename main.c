

/*
          [E]
[G]        :
 |         :  D (drift to the music?)
[F]--[B]--[C]
      |
     [A]



A: <port of fading memories> (pythonista)
B: <placeblocks> 
C: <you can jump in this room>
D: <tonebone: ZONES audio> 
E: TODO dialog system, if you bring the ??? here you get a spinning deer
F: TODO decide 
    - you can press E to pick up the shape and move it to other rooms. or maybe something else ?
    - "comprehensive magic system"
    - conversation room
G:

3D image board
*/

#include "raylib.h" /* 4.5.0 */
#include <rlgl.h>
#include <raymath.h>

#include <stdio.h>
#include <stdlib.h>

Camera camera = { 0 };
bool paused = false;
Vector2 look = {0., 0.};
bool mouse_ready = false;
#define JUMP_HEIGHT 6
#define JUMP_DURATION .5
float timer_jump = 0;

float timer_zone = 0;
Vector3 world_pos;

Model world;
Texture2D world_tex;

Sound Zone;

struct conversation_beat {
  char* question;
  Color color;
  char** responses;
  int* outcomes;
  bool relative;
  int responses_count;
};

// used for wall collisions
struct room_boundary {
  float MIN_X;
  float MAX_X;
  float MIN_Z;
  float MAX_Z;
  struct room_boundary* port_z_pos;
  struct room_boundary* port_z_neg;
  struct room_boundary* port_x_pos;
  struct room_boundary* port_x_neg;
  bool hidden; // from minimap/HUD
  void (*enter)(void);
  void (*resume)(void);
  void (*update)(void);
};

#define ROOM_COUNT 11

#define PLACEBLOCK_MAX 255
Vector3 placeblocks[PLACEBLOCK_MAX];
int placeblocks_count = 0;

// room to hallway distance is 24 units
struct room_boundary rooms[ROOM_COUNT] = {
  { -15-0*24,  15-0*24,  -15-0*24,  15-0*24, NULL, NULL, NULL, NULL, 0}, // 0 ROOM BLACK (A)
  {-1.9-0*24, 1.9-0*24,   -9-1*24,   9-1*24, NULL, NULL, NULL, NULL, 0}, // 1 HALL BLACK<>RED Z-DEPTH

  { -15-0*24,  15-0*24,  -15-2*24,  15-2*24, NULL, NULL, NULL, NULL, 0}, // 2 ROOM RED (B)
  {  -9-1*24,   9-1*24, -1.9-2*24, 1.9-2*24, NULL, NULL, NULL, NULL, 0}, // 3 HALL RED<>GREEN X-DEPTH
  {  -9+1*24,   9+1*24, -1.9-2*24, 1.9-2*24, NULL, NULL, NULL, NULL, 0}, // 4 HALL RED<>YELLOW X-DEPTH

  { -15-2*24,  15-2*24, -15-2*24, 15-2*24, NULL, NULL, NULL, NULL, 0}, // 5 ROOM GREEN (F)

  { -15+2*24,  15+2*24, -15-2*24, 15-2*24, NULL, NULL, NULL, NULL, 0}, // 6 ROOM YELLOW (C)
  { -15+2*24,  15+2*24, 15-6*24, -15-2*24, NULL, NULL, NULL, NULL, 1}, // 7 ROOM "ZONE"
  { -15+2*24,  15+2*24, -15-6*24, 15-6*24, NULL, NULL, NULL, NULL, 1}, // 8 ROOM BLANK

  {-1.9-2*24, 1.9-2*24,  -9-3*24,  9-3*24, NULL, NULL, NULL, NULL, 0}, // 9 HALL GREEN<>BLUE Z-DEPTH 

  { -15-2*24,  15-2*24, -15-4*24, 15-4*24, NULL, NULL, NULL, NULL, 0}, // 10 ROOM BLUE
};
struct room_boundary* room_current;

void init_rooms()
{
  // build room tree
  rooms[0].port_z_neg = &(rooms[1]); // BLACK->HALL
  rooms[1].port_z_pos = &(rooms[0]); // BLACK<-HALL
  rooms[1].port_z_neg = &(rooms[2]); // HALL->RED
  rooms[2].port_z_pos = &(rooms[1]); // HALL<-RED

  rooms[2].port_x_neg = &(rooms[3]); // RED->HALL(1)
  rooms[3].port_x_pos = &(rooms[2]); // RED<-HALL(1)
  rooms[2].port_x_pos = &(rooms[4]); // RED->HALL(2)
  rooms[4].port_x_neg = &(rooms[2]); // RED<-HALL(2)

  rooms[3].port_x_neg = &(rooms[5]); // HALL(1)->GREEN
  rooms[5].port_x_pos = &(rooms[3]); // HALL(1)<-GREEN

  rooms[4].port_x_pos = &(rooms[6]); // HALL(2)->YELLOW
  rooms[6].port_x_neg = &(rooms[4]); // HALL(2)<-YELLOW

  // link YELLOW to ZONE and blank
  rooms[6].port_z_neg = &(rooms[7]); // YELLOW->ZONE
  rooms[7].port_z_pos = &(rooms[6]); // YELLOW<-ZONE
  rooms[7].port_z_neg = &(rooms[8]); // ZONE->BLANK
  rooms[8].port_z_pos = &(rooms[7]); // ZONE->BLANK

  rooms[5].port_z_neg = &(rooms[9]); // GREEN->HALL
  rooms[9].port_z_pos = &(rooms[5]); // GREEN<-HALL
  rooms[9].port_z_neg = &(rooms[10]); // HALL->BLUE
  rooms[10].port_z_pos = &(rooms[9]); // HALL<-BLUE

  // initialize pointer to room tree
  room_current = &(rooms[0]);
}

void player_move(float x, float z)
{
  camera.position.x += x;
  camera.position.z += z;

  
  struct room_boundary* room_prev = room_current;

  // world boundaries
  if (camera.position.x < room_current->MIN_X)
    if (room_current->port_x_neg && 
        camera.position.z > room_current->port_x_neg->MIN_Z &&
        camera.position.z < room_current->port_x_neg->MAX_Z)
      room_current = room_current->port_x_neg;
    else
      camera.position.x = room_current->MIN_X;
  if (camera.position.x > room_current->MAX_X)
    if (room_current->port_x_pos && 
        camera.position.z > room_current->port_x_pos->MIN_Z &&
        camera.position.z < room_current->port_x_pos->MAX_Z)
      room_current = room_current->port_x_pos;
    else
      camera.position.x = room_current->MAX_X;
  if (camera.position.z < room_current->MIN_Z) 
    if (room_current->port_z_neg && 
        camera.position.x > room_current->port_z_neg->MIN_X &&
        camera.position.x < room_current->port_z_neg->MAX_X)
      room_current = room_current->port_z_neg;
    else
      camera.position.z = room_current->MIN_Z;
  if (camera.position.z > room_current->MAX_Z)
    if (room_current->port_z_pos && 
        camera.position.x > room_current->port_z_pos->MIN_X &&
        camera.position.x < room_current->port_z_pos->MAX_X)
      room_current = room_current->port_z_pos;
    else
      camera.position.z = room_current->MAX_Z;

    if (room_current != room_prev)
    {
      // change room event

      if (room_current == &rooms[7] && timer_zone < 26)
      {
        PlaySound(Zone);
      }
      else if (IsSoundPlaying(Zone)) 
      { 
        // reset only if didn't finish playing the first time
        if (timer_zone < 26) timer_zone = 0;

        world_pos.x = 0;
        world_pos.y = 0;
        world_pos.z = 0;
        StopSound(Zone);
      }
    }
}

void player_jump()
{
  if (IsKeyPressed(KEY_SPACE) && timer_jump == 0) timer_jump = JUMP_DURATION;
  if (timer_jump > 0) timer_jump -= GetFrameTime();
  else timer_jump = 0;
  // let's call this 'stylized jumping' instead of physically based
  camera.position.y = 5 + sin(timer_jump*PI/JUMP_DURATION) * JUMP_HEIGHT;
}

void rlVertex3v3f(Vector3 lx, Vector3 ly, Vector3 lz, float x, float y, float z)
{
  // extension to RL/GL API, preforms rlVertex3f call with arbitrary change of basis

  // to apply the rotation:
  // rlVertex3f(xf,yf,zf):
  //   Vector Math: C = xf*lx + yf*ly + zf*lz
  //   rlVertex3f(C.x, C.y, C.z);

  // It's possible for some transformations to invert normals
  // make sure triangles are still drawn counter-clockwise from the outside

  Vector3 C = Vector3Scale(lx, x);
  C = Vector3Add(C, Vector3Scale(ly, y));
  C = Vector3Add(C, Vector3Scale(lz, z));
  rlVertex3f(C.x, C.y, C.z);
}

void DrawThickLine3D(Vector3 start, Vector3 end, float thickness, Color color)
{
  // adapted from DrawCylinder in Raylib > rmodels.c
  rlPushMatrix();
  rlTranslatef(start.x, start.y, start.z);
  rlBegin(RL_TRIANGLES);
  rlColor4ub(color.r, color.g, color.b, color.a);

  // line local coordinate bases
    // direction of line
    Vector3 ly = Vector3Normalize(Vector3Subtract(end, start));
    // 2d plane to draw end caps (arbitrary rotation)
    Vector3 lz = Vector3Perpendicular(ly);
    Vector3 lx = Vector3CrossProduct(ly, lz);

    float length = Vector3Length(Vector3Subtract(end, start));


  // Draw Body 
  for (int i = 0; i < 360; i += 120)
  {
    // Draw quad using triangles

    // Triangle 0: Bottom Left
    rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*i)*thickness, 0, cosf(DEG2RAD*i)*thickness);

    // Triangle 0: Bottom Right
    rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*(i + 120))*thickness, 0, cosf(DEG2RAD*(i + 120))*thickness);

    // Triangle 0: Top Right
    rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*(i + 120))*thickness, length, cosf(DEG2RAD*(i + 120))*thickness);

    // Triangle 1: Top Right
    rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*(i + 120))*thickness, length, cosf(DEG2RAD*(i + 120))*thickness);

    // Triangle 1: Top Left
    rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*i)*thickness, length, cosf(DEG2RAD*i)*thickness);

    // Triangle 1: Bottom Left
    rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*i)*thickness, 0, cosf(DEG2RAD*i)*thickness);
  }

  // Draw Bottom
  rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*0)*thickness, 0, cosf(DEG2RAD*0)*thickness);
  rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*120)*thickness, 0, cosf(DEG2RAD*120)*thickness);
  rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*240)*thickness, 0, cosf(DEG2RAD*240)*thickness);

  // Draw Top 
  rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*0)*thickness, length, cosf(DEG2RAD*0)*thickness);
  rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*120)*thickness, length, cosf(DEG2RAD*120)*thickness);
  rlVertex3v3f(lx, ly, lz, sinf(DEG2RAD*240)*thickness, length, cosf(DEG2RAD*240)*thickness);

  rlEnd();
  rlPopMatrix();
}

void load()
{
  world = LoadModel("world.obj");
  world_tex = LoadTexture("atlas.png");

  //printf("Loading...\r\n");
  while (!IsModelReady(world));
  while (!IsTextureReady(world_tex));
  //printf("Done Loading!\r\n");

  // I use only a single material and texture
  SetMaterialTexture(world.materials, MATERIAL_MAP_DIFFUSE, world_tex);
}

void draw_world()
{
  // Draw world model with baked lightmap
  {
    float scale = 2;
    Color tint = WHITE;
    //world_pos.x += GetFrameTime();
    DrawModel(world, world_pos, scale, tint);
  }

  // Draw wireframe
  if (0) {
    float thickness = .1;
    Color color = BLACK;

    // Y axis wireframes
    for (int x=-1; x<=1; x+=2) for (int y=-1; y<=1; y+=2) 
    for (int r=0; r<=1; r++) {
      Vector3 start = {x*16, 0, y*16-48*r};
      Vector3 end = {x*16, 12, y*16-48*r};
      DrawThickLine3D(start, end, thickness, color);
    }

    // X axis wireframes
    for (int z=-1; z<=1; z+=2) for (int y=0; y<=1; y++)
    for (int r=0; r<=1; r++) {
      Vector3 start = {-16, 12*y, 16*z-48*r};
      Vector3 end = {16, 12*y, 16*z-48*r};
      DrawThickLine3D(start, end, thickness, color);
    }
  }
}

void player_look(float x, float y)
{
  look.x += x; 
  look.y += y; 

  float pitch_clamp = PI/2-.0001;
  if (look.y < -pitch_clamp) look.y = -pitch_clamp;
  if (look.y > pitch_clamp) look.y = pitch_clamp;
}

void first_person_controller()
{
  float speed = .5;

  Vector3 forward = {0, 0, 0};
  // compute forward look direction
  float r = cos(look.y);
  forward.x = -r*cos(look.x-PI/2);
  forward.y = -sin(look.y);
  forward.z = -r*sin(look.x-PI/2);
  
  //forward = Vector3Normalize(forward);

  Vector3 right = {0, 0, 0};
  // compute orthogonal complement to forward
  right.x = cos(look.x);
  right.y = 0;
  right.z = sin(look.x);

  //right = Vector3Normalize(right);

  Vector2 walkForward = {forward.x, forward.z};
  walkForward = Vector2Normalize(walkForward);

  if (IsKeyDown(KEY_W))
    player_move(speed*walkForward.x, speed*walkForward.y);
  if (IsKeyDown(KEY_S))
    player_move(speed*-walkForward.x, speed*-walkForward.y);
  if (IsKeyDown(KEY_A))
    player_move(speed*right.x, speed*right.z);
  if (IsKeyDown(KEY_D))
    player_move(speed*-right.x, speed*-right.z);

  float look_speed = .001;

  Vector2 look_input = {0, 0};
  Vector2 md = GetMouseDelta();

  // support mouse look
  if (mouse_ready)
  { 
    look_input.x += md.x;
    look_input.y += md.y;
  }

  // support ijkl look
  float ijkl_look_speed = 25;
  look_input.x += ijkl_look_speed * (((int)IsKeyDown(KEY_L)) - ((int)IsKeyDown(KEY_J)));
  look_input.y += ijkl_look_speed * (((int)IsKeyDown(KEY_K)) - ((int)IsKeyDown(KEY_I)));

  player_look(look_speed*look_input.x, look_speed*look_input.y);

  camera.target.x = camera.position.x + forward.x;
  camera.target.y = camera.position.y + forward.y;
  camera.target.z = camera.position.z + forward.z;

  // mouse is ready on the frame after the first md != <0,0>
  if (md.x != 0 | md.y != 0)
    mouse_ready = true;
}

void draw_HUD()
{
  int offX = 100;
  int offY = 200;
  for (int i=0; i<ROOM_COUNT; i++)
  {
    if (rooms[i].hidden) continue;

    // Draw a color-filled rectangle
    int posX = rooms[i].MIN_X;
    int posY = rooms[i].MIN_Z;
    int width = rooms[i].MAX_X - rooms[i].MIN_X;
    int height = rooms[i].MAX_Z - rooms[i].MIN_Z;
    Color color = {0, 0, 0, 100};
    DrawRectangle(offX+posX, offY+posY, width, height, color);
  }
  // highlight current room
  {
    int posX = room_current->MIN_X;
    int posY = room_current->MIN_Z;
    int width = room_current->MAX_X - room_current->MIN_X;
    int height = room_current->MAX_Z - room_current->MIN_Z;
    Color color = {0, 100, 180, 100};
    DrawRectangle(offX+posX, offY+posY, width, height, color);
  }
  Vector3 a = camera.position;
  Vector3 forward = camera.target; forward = Vector3Subtract(forward, a);
  forward.y = 0; forward = Vector3Normalize(forward);
  Vector3 up = {0, 1, 0}; forward = Vector3Scale(forward, 8);
  Vector3 right = Vector3CrossProduct(forward, up); right = Vector3Scale(right, .7);
  Vector3 b = a; b = Vector3Add(b, forward); b = Vector3Add(b, right);
  Vector3 c = a; c = Vector3Add(c, forward); c = Vector3Subtract(c, right);
  Vector2 A = {offX+a.x, offY+a.z};
  Vector2 B = {offX+b.x, offY+b.z};
  Vector2 C = {offX+c.x, offY+c.z};
  Color ghost = RAYWHITE; ghost.a = offX;

  DrawTriangle(A, B, C, ghost);
  DrawRectangle(offX+camera.position.x-2, offY+camera.position.z-2, 4, 4, RAYWHITE);
}

bool draw_pause_menu()
{
  Color transGray = BLACK; transGray.a = 150;
  DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), transGray);

  int y = GetScreenHeight()/2-120;
  DrawText("PAUSED", 100, 10+y, 60, WHITE); y += 120;
  DrawText("ESC: RESUME", 100, 10+y, 60, WHITE); y += 60;
  DrawText("X: EXIT",   100, 10+y, 60, WHITE); y += 60;
  DrawText("G: ACTIVATE GHOST MODE",   100, 10+y, 60, WHITE); y += 60;

  if (IsKeyPressed(KEY_G))
  {
    SetWindowOpacity(.1);
  }

  return IsKeyPressed(KEY_X);
}

bool place_block_existing(Vector3 cube)
{
  for (int p=0; p<placeblocks_count; p++)
    if (Vector3Equals(placeblocks[p], cube))
      return true;
  return false;
}

bool place_block_delete_existing(Vector3 cube)
{
  
  for (int p=0; p<placeblocks_count; p++)
    if (Vector3Equals(placeblocks[p], cube))
    {
      for (int i=p; i<placeblocks_count-1; i++)
        placeblocks[i] = placeblocks[i+1];
      placeblocks_count--;
      return true;
    }
  return false;
}

void place_block()
{

  int s = 4;

  // let player add new one
  Vector3 forward = camera.target; forward = Vector3Subtract(forward, camera.position);
  Vector3 cubepos = forward; cubepos = Vector3Scale(cubepos, 20);
  cubepos = Vector3Add(cubepos, camera.position);

  cubepos.x = s*(int)(cubepos.x/s+.5)-s/2;
  cubepos.y = s*(int)(cubepos.y/s+.5)-s/2; 
  cubepos.z = s*(int)(cubepos.z/s+.5)-s/2;

  // confine block placing to room 2
  while (cubepos.x < rooms[2].MIN_X) cubepos.x += s;
  while (cubepos.x > rooms[2].MAX_X) cubepos.x -= s;
  while (cubepos.y < 0) cubepos.y += s;
  while (cubepos.y > 12) cubepos.y -= s;
  while (cubepos.z < rooms[2].MIN_Z) cubepos.z += s;
  while (cubepos.z > rooms[2].MAX_Z) cubepos.z -= s;

  Color c = PINK;
  c.a = 100;
  if (place_block_existing(cubepos)) { c = BLUE; c.r*=.7;c.g*=.7;c.b*=.7;c.a*=.7; }
  DrawCube(cubepos, s, s, s, c);
  if ((IsMouseButtonPressed(0) || IsKeyPressed(KEY_SPACE)) && placeblocks_count < PLACEBLOCK_MAX)
  {
    if (!place_block_delete_existing(cubepos))
    {
      placeblocks[placeblocks_count] = cubepos;
      placeblocks_count ++;
    }
  }
}

/*
This is a C-port of fadingmemories.py

fadingmemories.py is an unpublished CG sketch I made on my iphone during a lunch break at work.
It is based on lovedeathandrobots.py which is also a lunch-time sketch.

lovedeathandrobots.py allows creation of arbitrary rotated/scaled/colored squares using a
series of sliding gestures primarily on the x-axis. This unusual input method significantly 
affects the manner of output.

fadingmemories.py is a fork that removes player-input and substitute random variables,
resulting in an animated self-construction that I found pretty mesmerizing to watch. 
*/
struct fm_Box {
  float x;
  float y;
  float r;
  float s;
  float a; // a,b,c,z are a color in [0,1] rgba space
  float b;
  float c;
  float z;
};
#define FM_BOX_MAX 100
struct fm_Box fm_boxes[FM_BOX_MAX];
int fm_boxes_count = 0;
float fm_timer = 0;
int fm_phase = -1;
float randf()
{
  // produce a random number [0,1)

  return (float) random() / 0x80000000; // [0, 2^31) / 2^31
}
void fm_delta(float dx, float dy)
{
  if (fm_phase == 0)
  	fm_boxes[fm_boxes_count-1].x += dx;
  if (fm_phase == 1)
  	fm_boxes[fm_boxes_count-1].y += dx;
  if (fm_phase == 2)
  	fm_boxes[fm_boxes_count-1].r += dx;
  if (fm_phase == 3)
  	fm_boxes[fm_boxes_count-1].s += dx;
  if (fm_phase == 4)
  {
  	fm_boxes[fm_boxes_count-1].b -= dx/200.;
  	fm_boxes[fm_boxes_count-1].c -= dy/200.;
  }
  if (fm_phase == 5)
  {
  	fm_boxes[fm_boxes_count-1].a -= dx/200.;
  	fm_boxes[fm_boxes_count-1].z -= dy/200.;
  }

  // not needed in original, add clamps to color components
  if (fm_boxes[fm_boxes_count-1].a < 0) fm_boxes[fm_boxes_count-1].a = 0;
  if (fm_boxes[fm_boxes_count-1].b < 0) fm_boxes[fm_boxes_count-1].b = 0;
  if (fm_boxes[fm_boxes_count-1].c < 0) fm_boxes[fm_boxes_count-1].c = 0;
  if (fm_boxes[fm_boxes_count-1].z < 0) fm_boxes[fm_boxes_count-1].z = 0;
}
void fading_memories()
{
  // Define a 2d drawing area using a center point and two basis vectors
  // implemented assuming bases are axis aligned
  Vector3 display_center = {0, 6, 16};
  Vector3 display_x = {1, 0, 0};
  Vector3 display_y = {0, 1, 0};

  // used to define which axis is the flat one
  Vector3 dim = {0, 0, 0};
  dim = Vector3Add(dim, display_x);
  dim = Vector3Add(dim, display_y);
  Vector3 ndim = {1, 1, 1};
  ndim = Vector3Subtract(ndim, dim);

  // the original is based in pixels, so we need to scale things down to 3D units
  int pp3d = 20;

  // This is Z relative to the display's local axis
  float z_offset = 0;
  for (struct fm_Box* fm_box = &fm_boxes[0]; fm_box <= &fm_boxes[fm_boxes_count-1]; fm_box++)
  {
    z_offset += .01;
    Vector3 pos = display_center;

    pos = Vector3Add(pos, Vector3Scale(display_x, fm_box->x/pp3d));
    pos = Vector3Add(pos, Vector3Scale(display_y, fm_box->y/pp3d));
    pos = Vector3Add(pos, Vector3Scale(ndim, -z_offset));
    Vector3 scales = {fm_box->s/pp3d, fm_box->s/pp3d, fm_box->s/pp3d};

    // don't scale on the flat axis
    if (dim.x == 0) scales.x = 1;
    if (dim.y == 0) scales.y = 1;
    if (dim.z == 0) scales.z = 1;

    Color c = {fm_box->a*255, fm_box->b*255, fm_box->c*255, fm_box->z*255};
    Vector3 zero = {0, 0, 0};
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(fm_box->r, ndim.x, ndim.y, ndim.z);
    DrawCube(zero, scales.x, scales.y, scales.z, c);
    rlPopMatrix();
  }

  if (fm_timer <= 0)
  {
    fm_phase ++;
    fm_phase %= 6;
    if (fm_phase == 0)
    {
      // add fm_box
      if (fm_boxes_count < FM_BOX_MAX)
      {
        struct fm_Box b = {0, 0, 0, 100, 1, 1, 1, 1};
        fm_boxes[fm_boxes_count++] = b;
      }
    }
    fm_timer = randf()*.6;
  }
  fm_timer -= GetFrameTime();
  float k = 7;
  fm_delta(k*randf(), k*randf());
}

int main(void)
{
  srandom(0xA51L);
  SetTraceLogLevel(LOG_ERROR);

  int width = GetScreenWidth();
  int height = GetScreenHeight();

  InitWindow(width, height, "game");
  SetWindowState(FLAG_FULLSCREEN_MODE);

  // don't close on 'ESC'
  SetExitKey(0);

  // initialize audio
  InitAudioDevice();
  //printf("Waiting for audio device...\r\n");
  while (!IsAudioDeviceReady());
  //printf("Loading sound media...\r\n");
  Zone = LoadSound("Zone.wav");
  while (!IsSoundReady(Zone));
  //printf("Done.\r\n");

  SetMasterVolume(.25);

  camera.position = (Vector3){ 0.0, 5.0, 0.0f };
  camera.target = (Vector3){ 0.0, 5.0, -1.0f };
  camera.up = (Vector3){ 0.0, 1.0, 0.0f };
  camera.fovy = 80.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  SetTargetFPS(60);
  DisableCursor();

  // load resources from file
  load();
  world_pos.x = 0;
  world_pos.y = 0;
  world_pos.z = 0;

  // initialize collisions
  init_rooms();

  while (!WindowShouldClose())
  {
    if (IsKeyPressed(KEY_ESCAPE)) { 
      paused = !paused;
      if (paused) 
      {
        EnableCursor();
        if (IsSoundPlaying(Zone)) PauseSound(Zone);
      }
      else 
      {
        DisableCursor();

        // Raylib doesn't resume a sound unless it was paused
        ResumeSound(Zone);
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera);

    draw_world();
    if (!paused) {

      // if player is in room zero, play my port of fading memories
      if (room_current == &rooms[0])
        fading_memories();

      // if player is in room C, they can jump
      // this should be called right before first_person_controller
      // so the look target can be correctly updated
      if (room_current == &rooms[6])
        player_jump();

      first_person_controller();

      int s = 4;
      // draw existing blocks (blue player-placed cubes)
      for (int p=0; p<placeblocks_count; p++)
        DrawCube(placeblocks[p], s, s, s, BLUE);
      // allow block placing from room 2 and connected hallways
      if (room_current == &rooms[2] ||
          room_current == &rooms[1] ||
          room_current == &rooms[3] ||
          room_current == &rooms[4]) place_block();


      // Zone movement to music
      if (room_current == &rooms[7])
      {
        // continuous event in 'Zone'
        timer_zone += GetFrameTime();

        if (timer_zone < 2) // SETUP (1)    1-2-3-4
        {
          if (fmod(timer_zone, .5f) < .4f)
            world_pos.x += 6*GetFrameTime();
        }
        else if (timer_zone < 3.8) // RESPONSE (1)    5-------
        {
           world_pos.x -= 6*4/3.8*GetFrameTime();
        }
        else if (timer_zone < 6) // SETUP (2)    1-2-3-4
        {
          if (fmod(timer_zone, .5f) < .4f)
            world_pos.x -= 6*GetFrameTime();
        }
        else if (timer_zone < 8) // RESPONSE (2)    5-------
        {
           world_pos.x += 6*GetFrameTime();
        }
        else if (timer_zone < 10) // SETUP (3)    1-2-3-4
        {
          if (fmod(timer_zone, .5f) < .4f)
            world_pos.y -= 10*GetFrameTime();
        }
        else if (timer_zone < 12) // RESPONSE (3)    5-------
        {
           world_pos.y += 10*GetFrameTime();
        }
        else if (timer_zone < 14) // SETUP (4)    1-2-3-4
        {
          if (fmod(timer_zone, .5f) < .4f)
            world_pos.y -= 10*GetFrameTime();
        }
        else if (timer_zone < 16) // RESPONSE (4)    5-------
        {
           world_pos.y += 10*GetFrameTime();
        }
        else if (timer_zone < 18) // SETUP (5)    1-2-3-4
        {
          if (fmod(timer_zone, .5f) < .4f)
            world_pos.x += 6*GetFrameTime();
        }
        else if (timer_zone < 20) // RESPONSE (5)    5-------
        {
           world_pos.x += 4*sin(6*GetFrameTime());
        }
        else if (timer_zone < 22) // SETUP (6)    1-2-3-4
        {
          if (fmod(timer_zone, .5f) < .4f)
            world_pos.z -= 6*GetFrameTime();
            world_pos.y += 6*GetFrameTime();
        }
        else if (timer_zone < 24) // RESPONSE (6)    5-------
        {
           world_pos.y -= 6*GetFrameTime();
        }
        else if (timer_zone < 26) // ==== RESET ====
        {
           world_pos.x *= .96; 
           world_pos.y *= .96; 
           world_pos.z *= .96; 
        }
        else
        {
           world_pos.x = 0; 
           world_pos.y = 0; 
           world_pos.z = 0; 
        }
      }
    }

    EndMode3D();

    draw_HUD();

    if (paused && draw_pause_menu()) break;

    if (!paused)
    {
      // green room = conversations
      if (room_current == &rooms[5])
      {
        DrawText("Press 'T' to confer with your associates.", 10, 10, 60, WHITE);
        /*
        OnPress(T): text_input = true;
        hello -> greetings, hello, salutations

        We would like to know if you can answer a question for us.

        > Yes < No

        ? -> Does a duck have wings? What is a duck? Do I have wings?
        What is a duck? Is a duck yellow? Do I have wings?
        Yes:                         a duck is       Yes
        No:
        I don't know:

        3 pos, 3 neg, mixed
        We are pleased with your responses
        We are displeased with your responses
        You seem just as confused as we are 
        */
      }
    }

    EndDrawing();
  }

  CloseWindow();
  CloseAudioDevice();

  return 0;
}
