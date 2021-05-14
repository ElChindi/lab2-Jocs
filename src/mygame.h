
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

class Player
{
public:
    Ship* ship;

    Player();
    //void move

};

class Ship : public EntityMesh
{
public:
    static std::vector<Ship*> ships;

    char type;

    Ship() {
        ships.push_back(this);
    }


    ~Ship() {
        auto it = std::find(ships.begin(), ships.end(), this);
        ships.erase(it);
    }

    static void renderAll();
    static void updateAll(float dt);
};

class Sea : public EntityMesh
{
public:
    
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
    
    static Scene* getInstance() {
        if (!world) {
            world = new Scene();
        }
        return world;
    }
    

   

};

#endif
