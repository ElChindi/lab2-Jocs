
#ifndef MYGAME_H
#define MYGAME_H

#include "includes.h"
#include "camera.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "shader.h"
#include "animation.h"

#define FLOOR_HEIGHT 0.32
#define MAX_DISTANCE 2000
#define ISLE_Y_OFFSET -1
#define DIST_BTW_ISLES 500
#define ISLE_TYPES 2

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
    float maxVelocity;
    float currentVelocity;

    Humanoid() {
        shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
        color = Vector4(1, 1, 1, 1);
        currentVelocity = 0;
    }
    void move(float dt);
    void increaseVelocity(float dt);
    void rotate(float dt, eRotation rot);
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
    static void updateAll(float dt);
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
    Sea() {};
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
    
    
    static Scene* getInstance() {
        if (!world) {
            world = new Scene();
        }
        return world;
    }

};

#endif
