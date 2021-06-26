
#ifndef MYGAME_H
#define MYGAME_H

#include "includes.h"
#include "camera.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "shader.h"
#include "animation.h"
#include "libs/bass.h"

#define FLOOR_HEIGHT 0.32
#define MAX_DISTANCE 2000
#define ISLE_Y_OFFSET -1
#define DIST_BTW_ISLES 500
#define ISLE_TYPES 6

//Globals

//Enums
enum eRotation { eclock, antieclock };

//Structs

//Classes
//----------------------------------------Entities----------------------------------------//
class Entity
{
    public:
        Entity() {
            model = Matrix44();
            scaleFactor = 1;
        } //constructor
        virtual ~Entity() {} //destructor
 
    //some attributes 
    std::string name; 
    Matrix44 model;
    float scaleFactor;
 
    //methods overwritten by derived classes
    virtual void render() = 0;
    virtual void update(float elapsed_time) = 0;

    //some useful methods...
    Vector3 getPosition() {
        return model.getTranslation();
    };
    void scale(float factor) {
        scaleFactor = factor;
        model.scale(factor, factor, factor);
    };
    
};

class EntityMesh : public Entity
{
    public:
        //Attributes of this class 
        Mesh* mesh;
        Texture* texture;
        Shader* shader;
        Vector4 color;
 
        //methods overwritten 
        void render();
        void update(float dt) {};

        void loadMeshAndTexture(const char* meshPath, const char* texturePath) {
            mesh = Mesh::Get(meshPath);
            texture = Texture::Get(texturePath);
        };
};

class Humanoid : public EntityMesh
{
public:
    static std::vector<Animation*> playerAnimations;
    static std::vector<Animation*> skeliAnimations;
    float maxVelocity;
    float currentVelocity;
    Animation* currAnimation;
    Animation* idle;

    Humanoid();
    void movePlayer(float dt);
    void moveEnemyTowardsPlayer(float dt);
    bool isNearPlayer();
    void increaseVelocity(float dt);
    void rotate(float dt, eRotation rot);

    //void attack();
    //void dodge();

    void render();
};

class Ship : public EntityMesh
{
public:
    static std::vector<Ship*> ships;

    char type;
    float maxVelocity;
    float currentVelocity;

    Ship() {
        shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
        color = Vector4(1, 1, 1, 1);
        currentVelocity = 0;
        ships.push_back(this);
    }


    ~Ship() {
        auto it = std::find(ships.begin(), ships.end(), this);
        ships.erase(it);
    }

    void increaseVelocity(float dt);
    void reduceVelocity(float dt);
    void rotate(float dt, eRotation rot);
    void move(float dt);

    static void renderAll();
    static void updateAll(float dt);
};

class Isle : public EntityMesh {
public:
    static std::vector<Isle*> isles;

    char type;
    std::vector<EntityMesh*> noCollisionableThings;
    std::vector<EntityMesh*> collisionableThings;
    std::vector<Humanoid*> enemies;

    Isle() {
        shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
        color = Vector4(1, 1, 1, 1);
        isles.push_back(this);
    }


    ~Isle() {
        auto it = std::find(isles.begin(), isles.end(), this);
        isles.erase(it);
    }
    void createStuff();
    void createEnemies(int n);
    Vector3 getNewEnemyPosition();
    

    bool isAboveIsle(Vector3 pos);
    bool isAboveIsle(EntityMesh* e) { return isAboveIsle(e->getPosition()); };
    static void createRandomIsles(int number, int minX, int maxX, int minZ, int maxZ);
    static void createRandomIsles(int number, int maxDist) { createRandomIsles(number, -maxDist, maxDist, -maxDist, maxDist); };
    static Vector3 getNewIslePosition(int minX, int maxX, int minZ, int maxZ);

    void renderEnemies() {
        for (Humanoid* enemy : enemies) {
            enemy->render();
        }
    };
    static void renderAll() {
        for (Isle* isle : isles) {
            isle->render();
        }
    };
    void updateEnemies(float dt) {
        for (Humanoid* enemy : enemies) {
            if (enemy->isNearPlayer()) enemy->increaseVelocity(dt);
            enemy->moveEnemyTowardsPlayer(dt);
        }
    };
};

class Player
{
public:
    Ship* ship;
    bool onShip;
    Humanoid* pirate;

    Player();
    void comeAshore();
    void comeAboard();
    bool getPlayerSpawn(Vector3& spawnPos); // Finds a place where the player can spawn in the isle
};

class Sea : public EntityMesh
{
public:
    float tiles;
    Sea() { this->tiles = 500; }
    Sea(float tiles) { this->tiles = tiles; };
    void render();

};

class Skybox : public EntityMesh
{
public:
    Skybox();
    void render();

};
//----------------------------------------ANIMATIONS----------------------------------------//
class AnimationLibrary {
public:
    Animation* idle;

        AnimationLibrary() {

            idle = Animation::Get("data/models/pirate/idle.skanim");
    }
};




//----------------------------------------STAGES----------------------------------------//
class Stage {
public:
	virtual void render() {};
	virtual void update(double dt) {};
};

class MainMenuStage : public Stage {
public:
    virtual void render();
    virtual void update(double dt);

};

class SeaStage : public Stage {
public:
	virtual void render();
	virtual void update(double dt);

};

class LandStage : public Stage {
public:
    virtual void render();
    virtual void update(double dt);

};

//----------------------------------------Scene----------------------------------------//
class Scene {
public:
    static Scene* world;
    Scene();

    Isle* currentIsle; //store current isle from list of isles
    Player* player;
    Sea sea;
    Skybox sky;

    Camera* staticCam;
    Camera* playingCam;
    Camera* cam2D;

    //for main menu bg
    Sea bgSea;
    Isle* bgIsle;
    Ship* bgShip;

    bool isPaused;
    
    
    static Scene* getInstance() {
        if (!world) {
            world = new Scene();
        }
        return world;
    }

};

//----------------------------------------AudioManager----------------------------------------//
class AudioManager {
public:
    static AudioManager* audio;
    AudioManager() {
        //Inicializamos BASS al arrancar el juego (id_del_device, muestras por segundo, ...)
        if (BASS_Init(-1, 44100, 0, 0, NULL) == false) //-1 significa usar el por defecto del sistema operativo
        {
            //error abriendo la tarjeta de sonido...
        }

        //El handler para un sample
        HSAMPLE hSample;

        //El handler para un canal
        HCHANNEL hSampleChannel;


    };
};

class GUI {
public:
    static bool usingMouse;
    static int currentButton;
    static const int nMenuButtons = 3;
    static const int nPauseButtons = 3;
    static void navigateMenu(int nButtons);
    static bool renderButton(int buttonNumber, float x, float y, float w, float h, Texture* tex, bool flipuvs);
    static void renderGradient();
    static void renderMainMenu();
    static void renderPauseMenu();
};


#endif
