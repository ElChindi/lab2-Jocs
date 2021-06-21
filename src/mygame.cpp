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
#include "libs/bass.h"

#include <cmath>

Scene* Scene::world = NULL;
std::vector<Ship*> Ship::ships;
std::vector<Isle*> Isle::isles;


//----------------------------------------Scene----------------------------------------//
Scene::Scene() {
	//load one texture without using the Texture Manager (Texture::Get would use the manager)

	//create isles
	Isle::createRandomIsles(20, MAX_DISTANCE);
	currentIsle = NULL; //change if we start in an isle
	
	//Initialize Player
	player = new Player();
	
	//Initialize Sea
	sea = Sea();
	sea.mesh = new Mesh();
	sea.mesh->createPlane(5000);
	sea.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_sea.fs");
	sea.color = Vector4(1, 1, 1, 0.8);
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
	
	Scene::world->player->ship->render();//cambiar por renderizar all ships

	Isle::renderAll();

	//Draw the floor grid
	drawGrid();
	Scene::world->sea.render();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

}

void SeaStage::update(double dt) {
	
	Scene::world->player->ship->move(dt);

	if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::gamepads[0].isButtonPressed(A_BUTTON)) Scene::world->player->ship->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::gamepads[0].direction & PAD_DOWN) Scene::world->player->ship->reduceVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::gamepads[0].direction & PAD_LEFT) Scene::world->player->ship->rotate(dt, antieclock);
	if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::gamepads[0].direction & PAD_RIGHT) Scene::world->player->ship->rotate(dt, eclock);
	if (Input::wasKeyPressed(SDL_SCANCODE_E) || Input::gamepads[0].wasButtonPressed(Y_BUTTON)) Scene::world->player->comeAshore();

	//camera follows ship with lerp

	Vector3 oldEye = Game::instance->camera->eye;
	Vector3 oldCenter = Game::instance->camera->eye;
	Vector3 newEye = (Scene::world->player->ship->model * Vector3(0, 20, 20) - oldEye) * 0.03 * dt * 100 + oldEye;
	Vector3 newCenter = (Scene::world->player->ship->model * Vector3(0, 0, -20) - oldCenter) * 0.1 * dt * 100 + oldCenter;
	Game::instance->camera->lookAt(newEye, newCenter, Vector3(0, 1, 0));
}
//----------------------------------------LANDSTAGE----------------------------------------//
void LandStage::render() {
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
	Scene::world->player->ship->render();//cambiar por renderizar all ships
	Scene::world->player->pirate->render();

	Isle::renderAll();
	Scene::world->currentIsle->renderEnemies();

	//Draw the floor grid
	drawGrid();
	Scene::world->sea.render();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

}

void LandStage::update(double dt) {

	Scene::world->player->pirate->move(dt);

	if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::gamepads[0].direction & PAD_UP) Scene::world->player->pirate->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::gamepads[0].direction & PAD_DOWN) Scene::world->player->pirate->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::gamepads[0].direction & PAD_LEFT) Scene::world->player->pirate->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::gamepads[0].direction & PAD_RIGHT) Scene::world->player->pirate->increaseVelocity(dt);
	if (Input::wasKeyPressed(SDL_SCANCODE_E) || Input::gamepads[0].wasButtonPressed(Y_BUTTON)) Scene::world->player->comeAboard();

	float rightAnalog = Input::gamepads[0].axis[Gamepad::RIGHT_ANALOG_X];

	if (rightAnalog > 0.2) 
	{ 
		Scene::world->player->pirate->rotate(dt * 4 * abs(rightAnalog), eclock); 
	}
	else if (Input::isKeyPressed(SDL_SCANCODE_RIGHT)) { Scene::world->player->pirate->rotate(dt * 3, eclock); }
	if (rightAnalog < -0.2) 
	{
		Scene::world->player->pirate->rotate(dt * 4 * abs(rightAnalog), antieclock); 
	}
	else if (Input::isKeyPressed(SDL_SCANCODE_LEFT)) { Scene::world->player->pirate->rotate(dt * 3, antieclock); }

	//camera follows pirate with lerp

	Vector3 oldEye = Game::instance->camera->eye;
	Vector3 oldCenter = Game::instance->camera->eye;
	Vector3 newEye = (Scene::world->player->pirate->model * Vector3(0, 3, 3) - oldEye) * 0.03 * dt * 200 + oldEye;
	Vector3 newCenter = (Scene::world->player->pirate->model * Vector3(0, 0, -10) - oldCenter) * 0.1 * dt * 100 + oldCenter;
	Game::instance->camera->lookAt(newEye, newCenter, Vector3(0, 1, 0));

}
//----------------------------------------Player----------------------------------------//
Player::Player() {
	onShip = true;

	ship = new Ship();
	ship->maxVelocity = 30;
	ship->scale(2);
	ship->loadMeshAndTexture("data/ship_light_cannon.obj", "data/ship_light_cannon.tga");


	pirate = new Humanoid();
	pirate->maxVelocity = 4;
	pirate->scale(3);
	pirate->loadMeshAndTexture("data/pirate.obj", "data/pirate.tga");
}

void Player::comeAshore()
// Switches game stage from SeaStage to LandStage
{
	if (ship->currentVelocity < 0.1) //to be adjusted
	{
		Vector3 spawnPosition;
		//spawnPosition = ship->model.getTranslation(); //DEBUG
		if (getPlayerSpawn(spawnPosition)) //if doesn't find anything
		{
			//float scale = pirate->model._11;
			pirate->model.setTranslation(spawnPosition.x, spawnPosition.y, spawnPosition.z);
			pirate->scale(pirate->scaleFactor);
			Game::instance->current_stage = 1;
		};
	}
};

void Player::comeAboard()
// Switches game stage from LandStage to SeaStage
{
	if ((pirate->getPosition() - ship->getPosition()).length() < 20) //to be adjusted
	{
		Scene::world->currentIsle = NULL;
		Game::instance->current_stage = 0;
	}
};


bool Player::getPlayerSpawn(Vector3& spawnPos) {
	Vector3 shipPos = ship->model.getTranslation() + Vector3(0,1,0);
	for (Isle* isle : Isle::isles) {
		Vector3 coll;
		Vector3 collnorm;
		//float isleScale = isle->model._11; //Suposing the scale is the same in xyz
		if (!isle->mesh->testSphereCollision(isle->model, shipPos, 0.4 /*/ isleScale*/, coll, collnorm)) //too far from isle?
			continue;
		//float pirateScale = Scene::world->player->pirate->model._11;
		collnorm = normalize(Vector3(coll.x - shipPos.x, 0.01, coll.z - shipPos.z));
		Vector3 trialSpawn = Vector3(coll.x+collnorm.x*3, FLOOR_HEIGHT*3/**pirateScale*/, coll.z+collnorm.z*3);
		if (isle->mesh->testSphereCollision(isle->model, trialSpawn + Vector3(0, 3, 0), 0.1 /*/ isleScale*/, coll, collnorm)) //player would collide?
			break;
		spawnPos = trialSpawn;
		Scene::world->currentIsle = isle;
		return true;
	}
	return false;//null?
}
//----------------------------------------Skybox----------------------------------------//
Skybox::Skybox() {
	loadMeshAndTexture("data/cielo.ASE", "data/cielo.tga");
	shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	color = Vector4(1, 1, 1, 1);
}

void Skybox::render() {
	model = Matrix44();
	model.scale(100, 100, 100);
	model.translate(Camera::current->eye.x / 100, Camera::current->eye.y / 100, Camera::current->eye.z / 100);
	EntityMesh::render();
}
//----------------------------------------Isle----------------------------------------//
void Isle::createEnemies(int n) {
	int enemiesLeft = n;
	while (enemiesLeft > 0) {
		Vector3 pos = getNewEnemyPosition();
		Humanoid* enemy = new Humanoid();
		this->enemies.push_back(enemy);
		enemy->model.setIdentity();
		enemy->scale(3);
		enemy->model.translateGlobal(pos.x, pos.y, pos.z);
		enemy->loadMeshAndTexture("data/pirate.obj", "data/pirate.tga");
		enemiesLeft -= 1;
	}
}

Vector3 Isle::getNewEnemyPosition() {
	bool validPos = false;
	Vector3 isleCenter = this->getPosition();
	Vector3 position;
	while (!validPos) {
		position = Vector3(random(DIST_BTW_ISLES, isleCenter.x - DIST_BTW_ISLES / 2), 1,
			random(DIST_BTW_ISLES, isleCenter.z - DIST_BTW_ISLES / 2));
		//supose it is valid
		validPos = true;
		//in isle and not colliding
		Vector3 coll;
		Vector3 collnorm;
		if (!isAboveIsle(position) || this->mesh->testSphereCollision(this->model, position + Vector3(0, 1.5, 0), 1.4 / this->scaleFactor, coll, collnorm)) {
			validPos = false;
			continue;
		}
	}
	return position;
}


bool Isle::isAboveIsle(Vector3 pos) {
	Vector3 coll;
	Vector3 collnorm;
	return mesh->testRayCollision(model, pos, Vector3(0, -1, 0), coll, collnorm,0.1);
}

void Isle::createRandomIsles(int number, int minX, int maxX, int minZ, int maxZ) {
	int islesLeft = number;
	while (islesLeft > 0) {
		Vector3 pos = getNewIslePosition(minX, maxX, minZ, maxZ);
		Isle* isle = new Isle();
		isle->model.setIdentity();
		isle->scale(30);
		isle->model.rotate(random(6.28), Vector3(0, 1, 0));
		isle->model.translateGlobal(pos.x, pos.y, pos.z);
		char type = rand() % ISLE_TYPES + 1;
		isle->type = type;
		isle->loadMeshAndTexture(("data/islas/"+std::to_string(type)+".obj").c_str(), ("data/islas/" + std::to_string(type) + ".tga").c_str());
		isle->createEnemies(10);
		islesLeft -= 1;
	}
}

Vector3 Isle::getNewIslePosition(int minX, int maxX, int minZ, int maxZ) {
	bool validPos = false;
	Vector3 position;
	while (!validPos) {
		//generate random pos within max distances + a padding
		position = Vector3(random(maxX - minX - DIST_BTW_ISLES, minX + DIST_BTW_ISLES / 2), ISLE_Y_OFFSET,
							random(maxZ - minZ - DIST_BTW_ISLES, minZ + DIST_BTW_ISLES / 2));
		//supose it is valid
		validPos = true;
		//it cant be near the center
		if (position.length() < DIST_BTW_ISLES) {
			validPos = false;
			continue; //try again
		}
		for (Isle* isle : isles) {
			if ((isle->getPosition() - position).length() < DIST_BTW_ISLES) {
				validPos = false;
				break; //try again
			}
		}
	}
	return position;
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
		model.rotate(0.5 * dt, Vector3(0, 1, 0));
	else
		model.rotate(-0.5 * dt, Vector3(0, 1, 0));
}

void Ship::move(float dt) {
	if (currentVelocity > 0) {
		model.translate(0, 0, -dt * currentVelocity);
		currentVelocity = clamp(currentVelocity - dt * 5, 0, maxVelocity);

		//Check collisions
		Vector3 targetCenter = model.getTranslation();
		for (EntityMesh* isle : Isle::isles) {
			Vector3 coll;
			Vector3 collnorm;
			//float scale = isle->model._11; //Suposing the scale is the same in xyz
			if (!isle->mesh->testSphereCollision(isle->model, targetCenter, 0.2, coll, collnorm))
				continue;

			if (currentVelocity > 5)
				currentVelocity = clamp(currentVelocity - dt * 100, 5, maxVelocity);
			Vector3 push_away = normalize(Vector3(coll.x - targetCenter.x, 0.001, coll.z - targetCenter.z)) * dt * currentVelocity;
			model.translateGlobal(-push_away.x * 2, 0, -push_away.z * 2);
		}
		//World border
		targetCenter = model.getTranslation() + Vector3(0, 1, 0);
		if (targetCenter.x < -MAX_DISTANCE)
			model.translateGlobal(-targetCenter.x - MAX_DISTANCE, 0, 0);
		if (targetCenter.x > MAX_DISTANCE)
			model.translateGlobal(-targetCenter.x + MAX_DISTANCE, 0, 0);
		if (targetCenter.z < -MAX_DISTANCE)
			model.translateGlobal(0, 0, -targetCenter.z - MAX_DISTANCE);
		if (targetCenter.z > MAX_DISTANCE)
			model.translateGlobal(0, 0, -targetCenter.z + MAX_DISTANCE);

	}
}
//----------------------------------------Humanoid----------------------------------------//
void Humanoid::move(float dt) {
	if (currentVelocity > 0) {
		Vector3 dir = Vector3();
		if ((Input::gamepads[0].direction & PAD_UP && Input::gamepads[0].direction & PAD_LEFT) || (Input::isKeyPressed(SDL_SCANCODE_W) && Input::isKeyPressed(SDL_SCANCODE_A))) {
			/*model.translate(0, 0, -dt * currentVelocity * 0.6);
			model.translate(-dt * currentVelocity * 0.6, 0, 0);*/
			dir = Vector3(-0.6, 0, -0.6);
		}
		else if ((Input::gamepads[0].direction & PAD_DOWN && Input::gamepads[0].direction & PAD_LEFT) || (Input::isKeyPressed(SDL_SCANCODE_S) && Input::isKeyPressed(SDL_SCANCODE_A))) {
			/*model.translate(0, 0, dt * currentVelocity * 0.3);
			model.translate(-dt * currentVelocity * 0.3, 0, 0);*/
			dir = Vector3(-0.3, 0, 0.3);
		}
		else if ((Input::gamepads[0].direction & PAD_UP && Input::gamepads[0].direction & PAD_RIGHT) || (Input::isKeyPressed(SDL_SCANCODE_W) && Input::isKeyPressed(SDL_SCANCODE_D))) {
			/*model.translate(0, 0, -dt * currentVelocity * 0.6);
			model.translate(dt * currentVelocity * 0.6, 0, 0);*/
			dir = Vector3(0.6, 0, -0.6);
		}
		else if ((Input::gamepads[0].direction & PAD_DOWN && Input::gamepads[0].direction & PAD_RIGHT) || (Input::isKeyPressed(SDL_SCANCODE_S) && Input::isKeyPressed(SDL_SCANCODE_D))) {
			/*model.translate(0, 0, dt * currentVelocity * 0.3);
			model.translate(dt * currentVelocity * 0.3, 0, 0);*/
			dir = Vector3(0.3, 0, 0.3);
		}
		else if(Input::gamepads[0].direction & PAD_UP || (Input::isKeyPressed(SDL_SCANCODE_W))){
			//model.translate(0, 0, -dt * currentVelocity); 
			dir = Vector3(0, 0, -1);
		}
		else if (Input::gamepads[0].direction & PAD_DOWN || (Input::isKeyPressed(SDL_SCANCODE_S))) {
			//model.translate(0, 0, dt * currentVelocity * 0.5);
			dir = Vector3(0, 0, 0.5);
		}
		else if (Input::gamepads[0].direction & PAD_LEFT || (Input::isKeyPressed(SDL_SCANCODE_A))) {
			//model.translate(-dt * currentVelocity * 0.75, 0, 0);
			dir = Vector3(-0.75, 0, 0);
		}
		else if (Input::gamepads[0].direction & PAD_RIGHT || (Input::isKeyPressed(SDL_SCANCODE_D))) {
			//model.translate(dt * currentVelocity * 0.75, 0, 0);
			dir = Vector3(0.75, 0, 0);
		}

		
		model.translate(dt * currentVelocity * dir.x, 0, dt * currentVelocity * dir.z);

		currentVelocity = clamp(currentVelocity - dt * 5, 0, maxVelocity);
		Isle* isle = Scene::world->currentIsle;
		Vector3 targetCenter = model.getTranslation() + Vector3(0, 1.5, 0);
		Vector3 coll;
		Vector3 collnorm;

		//Check if in the isle
		if (!isle->isAboveIsle(this) && isle->mesh->testSphereCollision(isle->model, targetCenter, 2 / isle->scaleFactor, coll, collnorm)) {
			//std::cout << "Out" << std::endl;
			Vector3 push_in = normalize(Vector3(coll.x - targetCenter.x, 0.000000001, coll.z - targetCenter.z)) * dt * currentVelocity * scaleFactor;
			model.translateGlobal(push_in.x, 0, push_in.z);
		}
			//model.translate(-dt * currentVelocity * dir.x, 0, -dt * currentVelocity * dir.z); 

		//Check collisions
		if (!isle->mesh->testSphereCollision(isle->model, targetCenter, 1.4/isle->scaleFactor, coll, collnorm))
			return;

		//if (currentVelocity > 5)
		//	currentVelocity = clamp(currentVelocity - dt * 100, 5, maxVelocity);
		Vector3 push_away = normalize(Vector3(coll.x - targetCenter.x, 0.000000001, coll.z - targetCenter.z)) * dt * currentVelocity * scaleFactor;
		model.translateGlobal(-push_away.x, 0, -push_away.z);
	}

}

void Humanoid::increaseVelocity(float dt) {

	currentVelocity = clamp(currentVelocity + dt * 10, 0, maxVelocity);
}

void Humanoid::rotate(float dt, eRotation rot) {
	if (rot == eclock)
		model.rotate(0.5 * dt, Vector3(0, 1, 0));
	else
		model.rotate(-0.5 * dt, Vector3(0, 1, 0));
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

