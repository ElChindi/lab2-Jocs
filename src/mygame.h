
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
#define MAX_DISTANCE 8000
#define ISLE_Y_OFFSET -1
#define DIST_BTW_ISLES 1000
#define APROX_ISLE_SIZE 500
#define ISLE_TYPES 6

#define MAX_PLAYER_HP 10
#define MAX_SKELI_HP 3

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

        EntityMesh() {
            shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
            color = Vector4(1, 1, 1, 1);
        }
 
        //methods overwritten 
        void render();
        void update(float dt) {};

        void loadMeshAndTexture(const char* meshPath, const char* texturePath) {
            mesh = Mesh::Get(meshPath);
            texture = Texture::Get(texturePath);
        };
};

class Sword : public EntityMesh
{
public:
    Matrix44 handMatrix;

    //Sword();

    void render();

};

class Humanoid : public EntityMesh
{
public:
    static std::vector<Animation*> playerAnimations;
    static std::vector<Animation*> skeliAnimations;
    float maxVelocity;
    float currentVelocity;
    Animation* currAnimation;
    float anim_time;
    bool attacking;
    bool dodging;
    bool hasHitSomeone;
    int hp;

    Humanoid();
    void movePlayer(float dt);

    void increaseVelocity(float dt);
    void rotate(float dt, eRotation rot);

    //void attack();
    //void dodge();

    void render();

    static void loadAnimations() {
        playerAnimations.reserve(6);
        playerAnimations.push_back(Animation::Get("data/models/pirate/idle.skanim"));
        playerAnimations.push_back(Animation::Get("data/models/pirate/runf.skanim"));
        playerAnimations.push_back(Animation::Get("data/models/pirate/runb.skanim"));
        playerAnimations.push_back(Animation::Get("data/models/pirate/attack.skanim"));
        playerAnimations.push_back(Animation::Get("data/models/pirate/dodge.skanim"));
        playerAnimations.push_back(Animation::Get("data/models/pirate/death.skanim"));

        skeliAnimations.reserve(4);
        skeliAnimations.push_back(Animation::Get("data/models/skeli/idle.skanim"));
        skeliAnimations.push_back(Animation::Get("data/models/skeli/run.skanim"));
        skeliAnimations.push_back(Animation::Get("data/models/skeli/attack.skanim"));
        skeliAnimations.push_back(Animation::Get("data/models/skeli/death.skanim"));

    }
};

class Skeli : public Humanoid
{
public:
    bool alive;
    bool moving;
    float attackTimer;


    Skeli() {
        alive = true;
        moving = false;
        attacking = false;
        hp = MAX_SKELI_HP;
    }
    void followPlayer(float dt);
    bool isNearPlayer(int radius);

    void initiateAttack();
    void attack(float dt);
    bool hitPlayer();
    //void initiateDie();   ??
    void die();
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

    void render();

    static void renderAll();
    static void updateAll(float dt);
};

class Isle : public EntityMesh {
public:
    static std::vector<Isle*> isles;

    char type;
    std::vector<EntityMesh*> noCollisionableThings;
    std::vector<EntityMesh*> collisionableThings;
    std::vector<Skeli*> enemies;
    bool enemiesLeft;
    Skeli* activeEnemy;

    Isle() {
        shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
        color = Vector4(1, 1, 1, 1);
        isles.push_back(this);
        activeEnemy = NULL;
    }


    ~Isle() {
        auto it = std::find(isles.begin(), isles.end(), this);
        isles.erase(it);
    }
    void createStuff();
    void createEnemies(int n);
    Vector3 getValidPosition();
    

    bool isAboveIsle(Vector3 pos);
    bool isAboveIsle(Vector3 pos, float padding);
    bool isAboveIsle(EntityMesh* e) { return isAboveIsle(e->getPosition()); };
    static void createRandomIsles(int number, int minX, int maxX, int minZ, int maxZ);
    static void createRandomIsles(int number, int maxDist) { createRandomIsles(number, -maxDist, maxDist, -maxDist, maxDist); };
    static Vector3 getNewIslePosition(int minX, int maxX, int minZ, int maxZ);

    void renderStuff() {
        for (EntityMesh* stuff : noCollisionableThings) {
            stuff->render();
        }
        for (EntityMesh* stuff : collisionableThings) {
            stuff->render();
        }
    };

    void renderEnemies() {
        for (Skeli* enemy : enemies) {
            if(enemy->alive) enemy->render();
        }
    };
    static void renderAll() {
        for (Isle* isle : isles) {
            isle->render();
        }
    };
    void updateEnemies(float dt);
};

class Player
{
public:
    Ship* ship;
    Humanoid* pirate;
    Sword* sword;

    int points;

    Player();
    void comeAshore();
    void comeAboard();
    void respawnPlayer();
    bool getPlayerSpawn(Vector3& spawnPos); // Finds a place where the player can spawn in the isle
    void initiateAttack();
    void attack(float dt);
    bool hitEnemy();
    void initiateDodge();
    void dodge(float dt);
    //void initiateDie();   ??
    void die();
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

    Isle* startingIsle;

    bool isPaused;

    HSAMPLE currentMusic;
    
    
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

    static std::map<std::string, HSAMPLE*> sSamplesLoaded;



    AudioManager() {
        //Inicializamos BASS al arrancar el juego (id_del_device, muestras por segundo, ...)
        if (BASS_Init(-1, 44100, 0, 0, NULL) == false) //-1 significa usar el por defecto del sistema operativo
        {
            //error abriendo la tarjeta de sonido...
        }
    };

    static AudioManager* getInstance() {
        if (!audio) {
            audio = new AudioManager();
        }
        return audio;
    }

    HSAMPLE play(const char* filename) {

        assert(filename);

        if (BASS_Init(-1, 44100, 0, 0, NULL) == false) //-1 significa usar el por defecto del sistema operativo
        {
            //error abriendo la tarjeta de sonido...
        }

        //El handler para un sample
        HSAMPLE hSample = NULL;
        //El handler para un canal
        HCHANNEL hSampleChannel = NULL;

        //check if loaded
        std::map<std::string, HSAMPLE*>::iterator it = sSamplesLoaded.find(filename);
        if (it != sSamplesLoaded.end())
        {
            hSampleChannel = BASS_SampleGetChannel(*it->second, false);
            BASS_ChannelPlay(hSampleChannel, true);
            return *it->second;
        }
        else
        {
            //load it

            HSAMPLE hSample = BASS_SampleLoad(false, filename, 0, 0, 20, 0);

            
            hSampleChannel = BASS_SampleGetChannel(hSample, false);
            sSamplesLoaded[filename] = &hSample;
            BASS_ChannelPlay(hSampleChannel, true);

            return hSample;
        };
    }
    HSAMPLE playloop(const char* filename) {
        assert(filename);

        if (BASS_Init(-1, 44100, 0, 0, NULL) == false) //-1 significa usar el por defecto del sistema operativo
        {
            //error abriendo la tarjeta de sonido...
        }

        //El handler para un sample
        HSAMPLE hSample = NULL;
        //El handler para un canal
        HCHANNEL hSampleChannel = NULL;

        //check if loaded
        std::map<std::string, HSAMPLE*>::iterator it = sSamplesLoaded.find(filename);
        if (it != sSamplesLoaded.end())
        {
            hSampleChannel = BASS_SampleGetChannel(*it->second, false);
            BASS_ChannelPlay(hSampleChannel, true);
            return *it->second;
        }
        else
        {
            //load it

            HSAMPLE hSample = BASS_SampleLoad(false, filename, 0, 0, 20, BASS_SAMPLE_LOOP);


            hSampleChannel = BASS_SampleGetChannel(hSample, false);
            sSamplesLoaded[filename] = &hSample;
            BASS_ChannelPlay(hSampleChannel, true);

            return hSample;
        };
        
    }

    void stop(HSAMPLE hSample) {
        HCHANNEL hSampleChannel = BASS_SampleGetChannel(hSample, false);
        BASS_ChannelPause(hSampleChannel);
    }

    void loadSamples() {
        HSAMPLE hSample;
        hSample = BASS_SampleLoad(false, "data/music/Battle.wav", 0, 0, 20, BASS_SAMPLE_LOOP);
        sSamplesLoaded["data/music/Battle.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/music/Island.wav", 0, 0, 20, BASS_SAMPLE_LOOP);
        sSamplesLoaded["data/music/Island.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/music/Onepiece.wav", 0, 0, 20, BASS_SAMPLE_LOOP);
        sSamplesLoaded["data/music/Onepiece.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/sounds/boneCrack1.wav", 0, 0, 20, 0);
        sSamplesLoaded["data/sounds/boneCrack1.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/sounds/boneCrack2.wav", 0, 0, 20, 0);
        sSamplesLoaded["data/sounds/boneCrack2.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/sounds/dodge.wav", 0, 0, 20, 0);
        sSamplesLoaded["data/sounds/dodge.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/sounds/hit.wav", 0, 0, 20, 0);
        sSamplesLoaded["data/sounds/hit.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/sounds/swing.wav", 0, 0, 20, 0);
        sSamplesLoaded["data/sounds/swing.wav"] = &hSample;
        hSample = BASS_SampleLoad(false, "data/sounds/sword.wav", 0, 0, 20, 0);
        sSamplesLoaded["data/sounds/sword.wav"] = &hSample;
    }

};

class GUI {
public:
    static bool usingMouse;
    static int currentButton;
    static const int nMenuButtons = 3;
    static const int nPauseButtons = 3;
    static void navigateMenu(int nButtons);
    static bool renderButton(int buttonNumber, float x, float y, float w, float h, Texture* tex, bool flipuvs);
    static void renderBar(float x, float y, float w, float h, Vector4 color);
    static void renderSkeliHPBar(Skeli* enemy);
    static void renderPlayerHPBar();
    static void renderAllHPBars();
    static void renderActiveEnemyHPBar();
    static void renderGradient();
    static void renderImage(float x, float y, float w, float h, Texture* tex, bool flipuvs);
    static void renderPlayerPoints();
    static void renderEnemiesLeftBar();
    static void renderMainMenu();
    static void renderPauseMenu();
};


#endif
