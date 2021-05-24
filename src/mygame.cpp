#include "mygame.h"
#include "game.h"
#include "utils.h"
#include "mesh.h"
#include "texture.h"
#include "fbo.h"
#include "shader.h"
#include "input.h"
#include "animation.h"
#include "camera.h"

#include <cmath>

//Globals

Mesh* mesh1 = NULL;
Texture* texture1 = NULL;
Shader* shader1 = NULL;
float angle1 = 0;

Scene* Scene::world = NULL;
std::vector<Ship*> Ship::ships;



Scene::Scene() {
	//load one texture without using the Texture Manager (Texture::Get would use the manager)

	cube = EntityMesh();
	Matrix44 m;
	m.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	cube.model = m;
	cube.texture = new Texture();
	cube.texture->load("data/texture.tga");
	cube.mesh = Mesh::Get("data/box.ASE");
	cube.shader = Shader::Get("data/shaders/skinning.vs", "data/shaders/texture.fs");
	cube.color = Vector4(1,1,1,1);


	// Using cubes as obstacles (replace with isles then)
	EntityMesh* cube1 = new EntityMesh();
	Matrix44 m1;
	m1.translate(-100, 0, -100);
	cube1->model = m1;
	cube1->texture = new Texture();
	cube1->texture->load("data/texture.tga");
	cube1->mesh = Mesh::Get("data/box.ASE");
	cube1->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	cube1->color = Vector4(1, 1, 1, 1);
	cubes.push_back(cube1);

	EntityMesh* cube2 = new EntityMesh();
	Matrix44 m2;
	m2.translate(0, -30, -150);
	cube2->model = m2;
	cube2->texture = new Texture();
	cube2->texture->load("data/texture.tga");
	cube2->mesh = Mesh::Get("data/box.ASE");
	cube2->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	cube2->color = Vector4(1, 1, 1, 1);
	cubes.push_back(cube2);
	
	player = new Player();
	
	sea = Sea();
	sea.mesh = new Mesh();
	sea.mesh->createPlane(10000);
	sea.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_sea.fs");
	sea.color = Vector4(1, 1, 1, 0.8);
	Matrix44 m3;
	//m3.translate(floor(Camera::current->eye.x / 100.0) * 100.0f, 0.0f, floor(Camera::current->eye.z / 100.0f) * 100.0f);
	m3.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	sea.model = m3;
	sea.texture = new Texture();
	sea.texture->load("data/sea.tga");

	sky = Skybox();
}

Player::Player() {
	ship = new Ship();
	ship->maxVelocity = 30;
	ship->currentVelocity = 0;
	onShip = true;

	Matrix44 m;
	m.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	ship->model = m;
	ship->texture = new Texture();
	ship->texture->load("data/ship_light_cannon.tga");
	ship->mesh = Mesh::Get("data/ship_light_cannon.obj");
	ship->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	ship->color = Vector4(1, 1, 1, 1);
}

Skybox::Skybox() {
	mesh = Mesh::Get("data/cielo.ASE");
	texture = Texture::Get("data/cielo.tga");
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	color = Vector4(1, 1, 1, 1);
}

void Skybox::render() {
	model = Matrix44();
	model.scale(100, 100, 100);
	model.translate(Camera::current->eye.x/100, Camera::current->eye.y/100, Camera::current->eye.z/100);
	EntityMesh::render();
}

//void Player::movePlayer() {
//
//}

void Ship::increaseVelocity(float dt) {
	currentVelocity = clamp(currentVelocity + dt * 10, 0, maxVelocity);
	//model.translate(0, 0, -dt * currentVelocity);
}

void Ship::reduceVelocity(float dt) {
	currentVelocity = clamp(currentVelocity - dt * 10, 0, maxVelocity);
	//model.translate(0, 0, -dt * currentVelocity);
}

void Ship::rotate(float dt, eRotation rot) {
	if(rot == eclock)
		model.rotate(1*dt, Vector3(0, 1, 0));
	else
		model.rotate(-1*dt, Vector3(0, 1, 0));
}

void Ship::move(float dt) {
	if (currentVelocity > 0) {
		model.translate(0, 0, -dt * currentVelocity);
		currentVelocity = clamp(currentVelocity - dt * 5, 0, maxVelocity);

		//Check collisions
		Vector3 targetCenter = model.getTranslation() + Vector3(0, 1, 0);
		for (EntityMesh* cube : Scene::world->cubes) {
			Vector3 coll;
			Vector3 collnorm;
			if (!cube->mesh->testSphereCollision(cube->model, targetCenter, 3, coll, collnorm))
				continue;

			Vector3 push_away = normalize(coll - targetCenter) * dt * currentVelocity;
			model.translateGlobal(-push_away.x, 0, -push_away.z);
		}
	}
}

void Sea::render()
{

	glLineWidth(1);
	glEnable(GL_BLEND);
	glDepthMask(false);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	shader->enable();

	shader->setUniform("u_color", color);
	shader->setUniform("u_model", model);
	shader->setUniform("u_camera_position", Camera::current->eye);
	shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
	shader->setUniform("u_texture", texture, 0);
	shader->setUniform("u_time", Game::instance->time);
	shader->setUniform("u_texture_tiling", (float)300);
	mesh->render(GL_TRIANGLES);
	glDisable(GL_BLEND);
	glDepthMask(true);
	shader->disable();
}

//PLAYSTAGE METHODS
void PlayStage::render() {
	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera as default
	Game::instance->camera->enable();

	//set flags
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	Scene::world->sky.render();
	
	Scene::world->player->ship->render();

	for (EntityMesh* cube : Scene::world->cubes) {
		cube->render();
	}

	//Draw the floor grid
	drawGrid();
	Scene::world->sea.render();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

}

void PlayStage::update(double dt) {
	
	Scene::world->player->ship->move(dt);

	if (Input::isKeyPressed(SDL_SCANCODE_W)) Scene::world->player->ship->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_S)) Scene::world->player->ship->reduceVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_A)) Scene::world->player->ship->rotate(dt, antieclock);
	if (Input::isKeyPressed(SDL_SCANCODE_D)) Scene::world->player->ship->rotate(dt, eclock);
}

//ENTITY METHODS
void EntityMesh::render()
{
	//Check Frustum
	//if(!camera->)
	//get the last camera that was activated
	Camera* camera = Camera::current;
	Matrix44 model = this->model;

	//enable shader and pass uniforms
	shader->enable();
	shader->setUniform("u_color", color);
	shader->setUniform("u_model", model);
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_texture", texture, 0);
	shader->setUniform("u_time", Game::instance->time);

	//render the mesh using the shader
	mesh->render(GL_TRIANGLES);

	//disable the shader after finishing rendering
	shader->disable();
}
