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


//----------------------------------------Scene----------------------------------------//
Scene::Scene() {
	//load one texture without using the Texture Manager (Texture::Get would use the manager)

	// Using cubes as name (replace with isles then)
	testIsle = new EntityMesh();
	Matrix44 m1;
	m1.translate(-100, -1, -100);
	m1.scale(30, 30, 30);
	testIsle->model = m1;
	testIsle->texture = new Texture();
	testIsle->texture->load("data/islas/2.tga");
	testIsle->mesh = Mesh::Get("data/islas/2.obj");
	testIsle->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
	testIsle->color = Vector4(1, 1, 1, 1);
	isles.push_back(testIsle);

	//Initialize Player
	player = new Player();
	
	//Initialize Sea
	sea = Sea();
	sea.mesh = new Mesh();
	sea.mesh->createPlane(5000);
	sea.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_sea.fs");
	sea.color = Vector4(1, 1, 1, 0.8);
	Matrix44 m3;
	m3.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	sea.model = m3;
	sea.texture = new Texture();
	sea.texture->load("data/sea.tga");

	//Initialize Skybox
	sky = Skybox();
}

//----------------------------------------SEASTAGE----------------------------------------//
void SeaStage::render() {
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

	for (EntityMesh* isle : Scene::world->isles) {
		isle->render();
	}

	//Draw the floor grid
	drawGrid();
	Scene::world->sea.render();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

}

void SeaStage::update(double dt) {
	
	Scene::world->player->ship->move(dt);

	if (Input::isKeyPressed(SDL_SCANCODE_W)) Scene::world->player->ship->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_S)) Scene::world->player->ship->reduceVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_A)) Scene::world->player->ship->rotate(dt, antieclock);
	if (Input::isKeyPressed(SDL_SCANCODE_D)) Scene::world->player->ship->rotate(dt, eclock);

	//camera follows ship with lerp

	Vector3 oldEye = Game::instance->camera->eye;
	Vector3 oldCenter = Game::instance->camera->eye;
	Vector3 newEye = (Scene::world->player->ship->model * Vector3(0, 20, 20) - oldEye) * 0.03 * dt * 100 + oldEye;
	Vector3 newCenter = (Scene::world->player->ship->model * Vector3(0, 0, -20) - oldCenter) * 0.1 * dt * 100 + oldCenter;
	Game::instance->camera->lookAt(newEye, newCenter, Vector3(0, 1, 0));
}

//----------------------------------------Player----------------------------------------//
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
	ship->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
	ship->color = Vector4(1, 1, 1, 1);

	pirate = new EntityMesh();
	Matrix44 m1;
	m1.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	pirate->model = m1;
	pirate->texture = new Texture();
	pirate->texture->load("data/pirate.tga");
	pirate->mesh = Mesh::Get("data/pirate.obj");
	pirate->shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_phong.fs");
	pirate->color = Vector4(1, 1, 1, 1);
}
//----------------------------------------Skybox----------------------------------------//
Skybox::Skybox() {
	mesh = Mesh::Get("data/cielo.ASE");
	texture = Texture::Get("data/cielo.tga");
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	color = Vector4(1, 1, 1, 1);
}

void Skybox::render() {
	model = Matrix44();
	model.scale(100, 100, 100);
	model.translate(Camera::current->eye.x / 100, Camera::current->eye.y / 100, Camera::current->eye.z / 100);
	EntityMesh::render();
}
//----------------------------------------Ship----------------------------------------//
void Ship::increaseVelocity(float dt) {
	currentVelocity = clamp(currentVelocity + dt * 10, 0, maxVelocity);
	//model.translate(0, 0, -dt * currentVelocity);
}

void Ship::reduceVelocity(float dt) {
	currentVelocity = clamp(currentVelocity - dt * 10, 0, maxVelocity);
	//model.translate(0, 0, -dt * currentVelocity);
}

void Ship::rotate(float dt, eRotation rot) {
	if (rot == eclock)
		model.rotate(1 * dt, Vector3(0, 1, 0));
	else
		model.rotate(-1 * dt, Vector3(0, 1, 0));
}

void Ship::move(float dt) {
	if (currentVelocity > 0) {
		model.translate(0, 0, -dt * currentVelocity);
		currentVelocity = clamp(currentVelocity - dt * 5, 0, maxVelocity);

		//Check collisions
		Vector3 targetCenter = model.getTranslation() + Vector3(0, 1, 0);
		for (EntityMesh* isle : Scene::world->isles) {
			Vector3 coll;
			Vector3 collnorm;
			float scale = isle->model._11; //Suposing the scale is the same in xyz
			if (!isle->mesh->testSphereCollision(isle->model, targetCenter, 3 / scale, coll, collnorm))
				continue;

			if (currentVelocity > 5)
				currentVelocity = clamp(currentVelocity - dt * 100, 5, maxVelocity);
			Vector3 push_away = normalize(Vector3(coll.x - targetCenter.x, 0.001, coll.z - targetCenter.z)) * dt * currentVelocity;
			model.translateGlobal(-push_away.x * 2, 0, -push_away.z * 2);
		}
		//Let it within the world
		targetCenter = model.getTranslation() + Vector3(0, 1, 0);
		if (targetCenter.x < -2000)
			model.translateGlobal(-targetCenter.x - 2000, 0, 0);
		if (targetCenter.x > 2000)
			model.translateGlobal(-targetCenter.x + 2000, 0, 0);
		if (targetCenter.z < -2000)
			model.translateGlobal(0, 0, -targetCenter.z - 2000);
		if (targetCenter.z > 2000)
			model.translateGlobal(0, 0, -targetCenter.z + 2000);

	}
}

//----------------------------------------Entity----------------------------------------//
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
	shader->setUniform("u_eye", camera->eye);

	//render the mesh using the shader
	mesh->render(GL_TRIANGLES);

	//disable the shader after finishing rendering
	shader->disable();
}
//----------------------------------------Sea----------------------------------------//
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
	shader->setUniform("u_texture_tiling", (float)500);
	mesh->render(GL_TRIANGLES);
	glDisable(GL_BLEND);
	glDepthMask(true);
	shader->disable();
}