
#ifndef MYGAME_H
#define MYGAME_H

#include "includes.h"
#include "camera.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "shader.h"
#include "animation.h"


//Globals

//Enums
enum eRotation { eclock, antieclock };

//Structs

//Classes
//----------------------------------------Entities----------------------------------------//
class Entity
{
    public:
        Entity() {} //constructor
        virtual ~Entity() {} //destructor
 
    //some attributes 
    std::string name; 
    Matrix44 model;
 
    //methods overwritten by derived classes
    virtual void render() = 0;
    virtual void update(float elapsed_time) = 0;

    //some useful methods...
    Vector3 getPosition(); 
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
};



class Ship : public EntityMesh
{
public:
    static std::vector<Ship*> ships;

    char type;
    float maxVelocity;
    float currentVelocity;

    Ship() {
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

class Player
{
public:
    Ship* ship;
    bool onShip;

    Player();
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

class PlayStage : public Stage {
public:
	virtual void render();
	virtual void update(double dt);
};

//----------------------------------------Scene----------------------------------------//
class Scene {
public:
    static Scene* world;
    Scene();

    EntityMesh cube;
    EntityMesh ship;
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
