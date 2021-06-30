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

std::vector<Animation*> Humanoid::playerAnimations;
std::vector<Animation*> Humanoid::skeliAnimations;
std::map<std::string, HSAMPLE*> AudioManager::sSamplesLoaded;

AudioManager* AudioManager::audio = NULL;
Scene* Scene::world = NULL;
std::vector<Ship*> Ship::ships;
std::vector<Isle*> Isle::isles;
bool GUI::usingMouse = false;
int GUI::currentButton = 0;

//----------------------------------------Scene----------------------------------------//
Scene::Scene() {


	//Load resources
	Humanoid::loadAnimations();
	AudioManager::getInstance()->loadSamples();

	//Initialize Player
	player = new Player();

	//Create sword
	player->sword = new Sword();
	player->sword->scale(1);
	player->sword->loadMeshAndTexture("data/models/pirate/sword/sword.obj", "data/models/pirate/sword/color-atlas-new.tga");
	

	//create isles
	Isle::createRandomIsles(100, MAX_DISTANCE);
	
	//Starting isle
	startingIsle = new Isle();
	startingIsle->model.setIdentity();
	startingIsle->scale(30);
	startingIsle->model.translateGlobal(0, ISLE_Y_OFFSET, 0);
	startingIsle->loadMeshAndTexture("data/islas/start.obj", "data/islas/start.tga");
	startingIsle->type = 5;
	startingIsle->createStuff();

	currentIsle = startingIsle;
	
	//Initialize Sea
	sea = Sea(500);
	sea.mesh = new Mesh();
	sea.mesh->createPlane(MAX_DISTANCE + 1000);
	sea.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_sea.fs");
	sea.color = Vector4(1, 1, 1, 0.8);
	sea.texture = Texture::Get("data/sea.tga");

	//Initialize Skybox
	sky = Skybox();

	//Initialize Cameras
	staticCam = new Camera();
	staticCam->lookAt(Vector3(-10.f, 20.f, 50.f), Vector3(10.f, 15.f, 45.f), Vector3(0.f, 1.f, 0.f));
	staticCam->setPerspective(70.f, Game::instance->window_width / (float)Game::instance->window_height, 0.1f, 10000.f);

	playingCam = new Camera();
	playingCam->lookAt(Vector3(-200.f, 50.f, 0.f), Vector3(0.f, 0.f, 0.f), Vector3(0.f, 1.f, 0.f));
	playingCam->setPerspective(70.f, Game::instance->window_width / (float)Game::instance->window_height, 0.1f, 10000.f);

	cam2D = new Camera();
	cam2D->setOrthographic(0, Game::instance->window_width, Game::instance->window_height, 0, -1, 1);

	//Main Menu stuff
	bgSea = Sea(50);
	bgSea.mesh = new Mesh();
	bgSea.mesh->createPlane(500);
	bgSea.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture_sea.fs");
	bgSea.color = Vector4(1, 1, 1, 0.8);
	bgSea.texture = Texture::Get("data/sea.tga");

	bgIsle = new Isle();
	auto it = std::find(Isle::isles.begin(), Isle::isles.end(), bgIsle);
	Isle::isles.erase(it); //remove bgIsle from the true isles list
	bgIsle->model.setIdentity();
	bgIsle->scale(30);
	bgIsle->model.translateGlobal(100, ISLE_Y_OFFSET, 0);
	bgIsle->loadMeshAndTexture("data/islas/1.obj", "data/islas/1.tga");
	bgIsle->type = 5;
	bgIsle->createStuff();

	bgShip = new Ship();
	bgShip->model.setTranslation(15, 0, 55);
	bgShip->scale(2);
	bgShip->loadMeshAndTexture("data/ship_light_cannon.obj", "data/ship_light_cannon.tga");

	isPaused = false;

	GUI::usingMouse = false;
	GUI::currentButton = 0;

	currentMusic = AudioManager::audio->playloop("data/music/Island.wav");

}

//----------------------------------------GUI-----------------------------------------------//

void GUI::navigateMenu(int nButtons) {
	if (Input::wasKeyPressed(SDL_SCANCODE_W) || Input::wasKeyPressed(SDL_SCANCODE_UP) || Input::gamepads[0].direction & PAD_UP) {
		GUI::usingMouse = false;
		GUI::currentButton = clamp(GUI::currentButton - 1, 0, nButtons - 1);
	}
	if (Input::wasKeyPressed(SDL_SCANCODE_S) || Input::wasKeyPressed(SDL_SCANCODE_DOWN) || Input::gamepads[0].direction & PAD_DOWN) {
		GUI::usingMouse = false;
		GUI::currentButton = clamp(GUI::currentButton + 1, 0, nButtons - 1);
	}
}

bool GUI::renderButton(int buttonNumber, float x, float y, float w, float h, Texture* tex, bool flipuvs) {
	Mesh quad;
	quad.createQuad(x, y, w, h, flipuvs);

	Vector2 mouse = Input::mouse_position;
	float halfW = w * 0.5;
	float halfH = h * 0.5;
	float minX = x - halfW;
	float minY = y - halfH;
	float maxX = x + halfW;
	float maxY = y + halfH;

	bool hover = mouse.x > minX && mouse.y > minY && mouse.x < maxX && mouse.y < maxY;
	if (hover) {
		GUI::usingMouse = true;
		GUI::currentButton = buttonNumber;
	}
	bool focus = hover || (!GUI::usingMouse && GUI::currentButton == buttonNumber);
	bool pressed = GUI::usingMouse && Input::isMousePressed(SDL_BUTTON_LEFT) || 
		!GUI::usingMouse && (Input::gamepads[0].isButtonPressed(A_BUTTON) || Input::isKeyPressed(SDL_SCANCODE_RETURN));

	Vector4 normalColor = Vector4(1, 1, 1, 0.6);
	Vector4 hoverColor = Vector4(1, 1, 1, 1);

	Shader* shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	shader->enable();
	shader->setUniform("u_color", focus ? hoverColor : normalColor);
	Matrix44 quadModel;
	shader->setUniform("u_model", quadModel);
	shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
	shader->setUniform("u_texture", tex, 0);
	shader->setUniform("u_eye", Camera::current->eye);

	quad.render(GL_TRIANGLES);
	shader->disable();

	return focus && pressed;
}

void GUI::renderBar(float x, float y, float w, float h, Vector4 color) {
	Mesh quad;
	quad.createQuad(x, y, w, h, true);
	
	Scene::world->cam2D->enable();

	Shader* shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	shader->enable();
	shader->setUniform("u_color", color);
	Matrix44 quadModel;
	shader->setUniform("u_model", quadModel);
	shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
	shader->setUniform("u_texture", Texture::Get("data/GUI/null.tga"), 0);
	shader->setUniform("u_eye", Camera::current->eye);

	quad.render(GL_TRIANGLES);
	shader->disable();
}

void GUI::renderSkeliHPBar(Skeli* enemy) {
	if (enemy->hp <= 0) return;
	Vector3 bar_pos = enemy->getPosition() + Vector3(0, 4.25, 0);
	Vector3 projected_pos = Camera::current->project(bar_pos, Game::instance->window_width, Game::instance->window_height);
	if (projected_pos.z > 1) return; //as it is projected in the other side of the camera
	float distToCam = (bar_pos - Camera::current->eye).length(); //use distance to camera
	if (distToCam == 0) return;
	renderBar(projected_pos.x, Game::instance->window_height - projected_pos.y, 150 * enemy->hp / distToCam, 150 / distToCam, Vector4(1, 0, 0, 0.5));
}

void GUI::renderPlayerHPBar() {
	int hp = Scene::world->player->pirate->hp;
	if (hp <= 0) return;
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int size = 20;
	renderBar(50 + size * hp / 2.0, Game::instance->window_height - size, size * hp, size, Vector4(1, 0, 0, 0.5));
	drawText(15, Game::instance->window_height - size*1.5 + 2, "HP", Vector3(1, 0, 0), 2);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void GUI::renderAllHPBars() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (Skeli* enemy : Scene::world->currentIsle->enemies) {
		renderSkeliHPBar(enemy);
	};

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void GUI::renderActiveEnemyHPBar() {
	Skeli* enemy = Scene::world->currentIsle->activeEnemy;
	if (enemy == NULL || enemy->hp <= 0) return;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderSkeliHPBar(enemy);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void GUI::renderGradient() {
	int xCenter = Game::instance->window_width / 2;
	int yCenter = Game::instance->window_height / 2;

	Mesh gradientBlack;
	gradientBlack.createQuad(xCenter, yCenter, Game::instance->window_width, Game::instance->window_height, false);

	Shader* shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	shader->enable();
	shader->setUniform("u_color", Vector4(0, 0, 0, 0.2));
	Matrix44 quadModel;
	shader->setUniform("u_model", quadModel);
	shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
	shader->setUniform("u_eye", Camera::current->eye);

	gradientBlack.render(GL_TRIANGLES);
}

void GUI::renderImage(float x, float y, float w, float h, Texture* tex, bool flipuvs) {
	Mesh quad;
	quad.createQuad(x, y, w, h, flipuvs);

	Scene::world->cam2D->enable();

	Shader* shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	shader->enable();
	shader->setUniform("u_color", Vector4(1,1,1,1));
	Matrix44 quadModel;
	shader->setUniform("u_model", quadModel);
	shader->setUniform("u_viewprojection", Camera::current->viewprojection_matrix);
	shader->setUniform("u_texture", tex, 0);
	shader->setUniform("u_eye", Camera::current->eye);

	quad.render(GL_TRIANGLES);
	shader->disable();
}

void GUI::renderPlayerPoints() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int points = Scene::world->player->points;
	int size = 20;
	drawText(15, 20, "POINTS", Vector3(1, 0.4, 0), 2);
	drawText(100, 20, std::to_string(points), Vector3(0, 0, 0), 2);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void GUI::renderEnemiesLeftBar() {
	if (!Scene::world->currentIsle->enemiesLeft) return;
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int nEnemiesLeft = 0;
	for (Skeli* enemy : Scene::world->currentIsle->enemies)
		if (enemy->alive) nEnemiesLeft++;

	if (nEnemiesLeft == 0) {
		Scene::world->currentIsle->enemiesLeft = false;
		return;
	}
	
	int size = 20;
	renderBar(590 + size * nEnemiesLeft / 2.0, Game::instance->window_height - size, size * nEnemiesLeft, size, Vector4(0, 0, 1, 0.5));
	drawText(510, Game::instance->window_height - size * 1.5 + 2, "SKELIS", Vector3(0, 0, 1), 2);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void GUI::renderMainMenu() {
	Scene::world->cam2D->enable();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int xCenter = Game::instance->window_width / 2;
	//int yCenter = Game::instance->window_height / 2;

	renderGradient();
	renderImage(xCenter - 175, 200, 329, 205, Texture::Get("data/GUI/skulls_of_the_sea.tga"), true);

	if (renderButton(0, xCenter - 200, 400 + 45 * 0, 250, 30, Texture::Get("data/GUI/startGame.tga"), true)) {
		Game::instance->current_stage = 2;
		AudioManager::audio->stop(Scene::world->currentMusic);
		Scene::world->currentMusic = AudioManager::audio->playloop("data/music/Island.wav");
	}
	renderButton(1, xCenter - 200, 400 + 45 * 1, 250, 30, Texture::Get("data/GUI/configuration.tga"), true);
	if (renderButton(2, xCenter - 200, 400 + 45 * 2, 250, 30, Texture::Get("data/GUI/exit.tga"), true)) {
		Game::instance->must_exit = true;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void GUI::renderPauseMenu() {
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	int xCenter = Game::instance->window_width / 2;
	//int yCenter = Game::instance->window_height / 2;

	renderGradient();

	if (renderButton(0, xCenter + 200, 400 + 45 * 0, 250, 30, Texture::Get("data/GUI/resumeGame.tga"), true)) {
		Scene::world->isPaused = false;
	}
	renderButton(1, xCenter + 200, 400 + 45 * 1, 250, 30, Texture::Get("data/GUI/configuration.tga"), true);
	if (renderButton(2, xCenter + 200, 400 + 45 * 2, 250, 30, Texture::Get("data/GUI/backToMenu.tga"), true)) {
		Scene::world->isPaused = false;
		Game::instance->current_stage = 0;
		GUI::usingMouse = true;
		GUI::currentButton = 0;
		AudioManager::audio->stop(Scene::world->currentMusic);
		Scene::world->currentMusic = AudioManager::audio->playloop("data/music/Island.wav");
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

//----------------------------------------MAINMENUSTAGE----------------------------------------//

void MainMenuStage::render() {

	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set flags
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	Scene::world->staticCam->enable();
	Scene::world->sky.render();
	Scene::world->bgSea.render();
	Scene::world->bgIsle->render();
	Scene::world->bgIsle->renderStuff();
	Scene::world->bgShip->render();


	//Render buttons
	GUI::renderMainMenu();
}

void MainMenuStage::update(double dt) {
	GUI::navigateMenu(GUI::nMenuButtons);
}

//----------------------------------------SEASTAGE----------------------------------------//
void SeaStage::render() {
	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera as default
	//Game::instance->camera->enable();
	Scene::world->playingCam->enable();

	//set flags
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	Scene::world->sky.render();

	Scene::world->player->ship->render();//cambiar por renderizar all ships

	Isle::renderAll();
	//Scene::world->currentIsle->renderStuff();

	//Draw the floor grid
	//drawGrid();
	Scene::world->sea.render();
	GUI::renderPlayerPoints();

	//render the FPS, Draw Calls, etc
	//drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

	if (Scene::world->isPaused) {
		Scene::world->cam2D->enable();
		GUI::renderPauseMenu();
	}
}

void SeaStage::update(double dt) {
	if (Input::wasKeyPressed(SDL_SCANCODE_P) || Input::gamepads[0].wasButtonPressed(START_BUTTON)) {
		Scene::world->isPaused = !Scene::world->isPaused;
		GUI::usingMouse = false;
		GUI::currentButton = 0;
	}
	if (Scene::world->isPaused) {
		GUI::navigateMenu(GUI::nPauseButtons);
		return;
	}
	
	Scene::world->player->ship->move(dt);

	if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::gamepads[0].isButtonPressed(A_BUTTON)) Scene::world->player->ship->increaseVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::gamepads[0].isButtonPressed(B_BUTTON)) Scene::world->player->ship->reduceVelocity(dt);
	if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::gamepads[0].direction & PAD_LEFT) Scene::world->player->ship->rotate(dt, antieclock);
	if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::gamepads[0].direction & PAD_RIGHT) Scene::world->player->ship->rotate(dt, eclock);
	if (Input::wasKeyPressed(SDL_SCANCODE_E) || Input::gamepads[0].wasButtonPressed(Y_BUTTON)) Scene::world->player->comeAshore();

	//camera follows ship with lerp
	Scene::world->playingCam->enable();
	Vector3 oldEye = Camera::current->eye;
	Vector3 oldCenter = Camera::current->center;
	Vector3 newEye = (Scene::world->player->ship->model * Vector3(0, 20, 20) - oldEye) * 0.03 * dt * 100 + oldEye;
	Vector3 newCenter = (Scene::world->player->ship->model * Vector3(0, 0, -20) - oldCenter) * 0.1 * dt * 100 + oldCenter;
	assert((newEye.x != newCenter.x || newEye.y != newCenter.y || newEye.z != newCenter.z) && "Eye should be different to Center");
	Camera::current->lookAt(newEye, newCenter, Vector3(0, 1, 0));
}
//----------------------------------------LANDSTAGE----------------------------------------//
void LandStage::render() {
	//set the clear color (the background color)
	glClearColor(0.0, 0.0, 0.0, 1.0);

	// Clear the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set the camera as default
	//Game::instance->camera->enable();
	Scene::world->playingCam->enable();

	//set flags
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	Scene::world->sky.render();
	Scene::world->player->ship->render();//cambiar por renderizar all ships
	Scene::world->player->pirate->render();
	Scene::world->player->sword->render();
	Isle::renderAll();
	Scene::world->currentIsle->renderEnemies();
	Scene::world->currentIsle->renderStuff();

	//Draw the floor grid
	//drawGrid();
	Scene::world->sea.render();

	//render the FPS, Draw Calls, etc
	//drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);
	
	//GUI::renderAllHPBars();
	GUI::renderActiveEnemyHPBar();
	GUI::renderPlayerHPBar();
	GUI::renderPlayerPoints();
	GUI::renderEnemiesLeftBar();

	if (Scene::world->isPaused) {
		Scene::world->cam2D->enable();
		GUI::renderPauseMenu();
	}
}

void LandStage::update(double dt) {
	if (Input::wasKeyPressed(SDL_SCANCODE_P) || Input::gamepads[0].wasButtonPressed(START_BUTTON)) {
		Scene::world->isPaused = !Scene::world->isPaused;
		GUI::usingMouse = false;
		GUI::currentButton = 0;
	}
	if (Scene::world->isPaused) {
		GUI::navigateMenu(GUI::nPauseButtons);
		return;
	}

	//kill player if 0 hp
	if (Scene::world->player->pirate->hp <= 0) Scene::world->player->die();
	//update sword position
	Scene::world->player->sword->handMatrix = Scene::world->player->pirate->currAnimation->skeleton.getBoneMatrix("mixamorig_RightHand", false);
	
	Matrix44 mat = Scene::world->player->pirate->model;
	mat.rotate(180.0f * DEG2RAD, Vector3(0, 1, 0));
	Scene::world->player->sword->model = Scene::world->player->sword->handMatrix * mat;
	
	Scene::world->currentIsle->updateEnemies(dt);

	//Inputs

	Scene::world->player->attack(dt);
	Scene::world->player->dodge(dt);

	if (!Scene::world->player->pirate->attacking && !Scene::world->player->pirate->dodging) {
		Scene::world->player->pirate->movePlayer(dt);
		/*if (Input::isKeyPressed(SDL_SCANCODE_W) || Input::gamepads[0].direction & PAD_UP) Scene::world->player->pirate->increaseVelocity(dt);
		if (Input::isKeyPressed(SDL_SCANCODE_S) || Input::gamepads[0].direction & PAD_DOWN) Scene::world->player->pirate->increaseVelocity(dt);
		if (Input::isKeyPressed(SDL_SCANCODE_A) || Input::gamepads[0].direction & PAD_LEFT) Scene::world->player->pirate->increaseVelocity(dt);
		if (Input::isKeyPressed(SDL_SCANCODE_D) || Input::gamepads[0].direction & PAD_RIGHT) Scene::world->player->pirate->increaseVelocity(dt);*/
		if (Input::wasKeyPressed(SDL_SCANCODE_E) || Input::gamepads[0].wasButtonPressed(Y_BUTTON)) Scene::world->player->comeAboard();
		if (Input::wasKeyPressed(SDL_SCANCODE_SPACE) || Input::gamepads[0].wasButtonPressed(X_BUTTON)) Scene::world->player->initiateAttack();
		if (Input::wasKeyPressed(SDL_SCANCODE_Q) || Input::gamepads[0].wasButtonPressed(B_BUTTON)) Scene::world->player->initiateDodge();
	}

	float rightAnalogx = Input::gamepads[0].axis[Gamepad::RIGHT_ANALOG_X];
	float rightAnalogy = Input::gamepads[0].axis[Gamepad::RIGHT_ANALOG_Y];

	if (rightAnalogx > 0.2) 
	{ 
		Scene::world->player->pirate->rotate(dt * 4 * abs(rightAnalogx), eclock); 
	}
	else if (Input::isKeyPressed(SDL_SCANCODE_RIGHT)) { Scene::world->player->pirate->rotate(dt * 3, eclock); }
	if (rightAnalogx < -0.2) 
	{
		Scene::world->player->pirate->rotate(dt * 4 * abs(rightAnalogx), antieclock); 
	}
	else if (Input::isKeyPressed(SDL_SCANCODE_LEFT)) { Scene::world->player->pirate->rotate(dt * 3, antieclock); }

	if (rightAnalogy > 0.2)
	{
		//Camera::current->move(Vector3(0.0f, 0.0f, 1.0f));
	}


	//camera follows pirate with lerp
	Scene::world->playingCam->enable();

	Vector3 oldEye = Camera::current->eye;
	Vector3 oldCenter = Camera::current->center;
	Vector3 newEye = (Scene::world->player->pirate->model * Vector3(0, 3, 3) - oldEye) * 0.03 * dt * 200 + oldEye;
	Vector3 newCenter = (Scene::world->player->pirate->model * Vector3(0, 0, -10) - oldCenter) * 0.1 * dt * 100 + oldCenter;
	assert((newEye.x != newCenter.x || newEye.y != newCenter.y || newEye.z != newCenter.z) && "Eye should be different to Center");
	Camera::current->lookAt(newEye, newCenter, Vector3(0, 1, 0));

}
//----------------------------------------Player----------------------------------------//
Player::Player() {
	points = 0;

	ship = new Ship();
	ship->model.setTranslation(126, 0, -12);
	ship->model.rotate(2.1, Vector3(0, 1, 0));
	ship->maxVelocity = 40;
	ship->scale(2);
	ship->loadMeshAndTexture("data/ship_light_cannon.obj", "data/ship_light_cannon.tga");


	pirate = new Humanoid();
	pirate->maxVelocity = 5;
	pirate->model.setTranslation(-100, FLOOR_HEIGHT * 3, -20);
	pirate->model.rotate(1.5, Vector3(0, 1, 0));
	pirate->scale(2);
	pirate->loadMeshAndTexture("data/models/pirate/pirate.mesh", "data/models/pirate/pineapple-32-32x.tga");

	pirate->currAnimation = pirate->playerAnimations[0];

	pirate->attacking = false;
	pirate->dodging = false;
	pirate->hp = MAX_PLAYER_HP;
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
			Vector3 isleCenter = Scene::world->currentIsle->getPosition();
			Vector3 dir = Vector3(spawnPosition.x - isleCenter.x, 0, spawnPosition.z - isleCenter.z);
			if (dir.x == 0 && dir.z == 0) return;
			pirate->model.setTranslation(spawnPosition.x, spawnPosition.y, spawnPosition.z);
			pirate->model.setFrontAndOrthonormalize(dir);
			pirate->model.setUpAndOrthonormalize(Vector3(0, 1, 0));
			pirate->scale(pirate->scaleFactor);
			Game::instance->current_stage = 2;

			AudioManager::audio->stop(Scene::world->currentMusic);
			if(Scene::world->currentIsle == Scene::world->startingIsle || !Scene::world->currentIsle->enemiesLeft)
				Scene::world->currentMusic = AudioManager::audio->playloop("data/music/Island.wav");
			else
				Scene::world->currentMusic = AudioManager::audio->playloop("data/music/Battle.wav");
		};
	}
};

void Player::comeAboard()
// Switches game stage from LandStage to SeaStage
{
	if ((pirate->getPosition() - ship->getPosition()).length() < 20) //to be adjusted
	{
		Scene::world->currentIsle = NULL;
		Game::instance->current_stage = 1;
		AudioManager::audio->stop(Scene::world->currentMusic);
		Scene::world->currentMusic = AudioManager::audio->playloop("data/music/Onepiece.wav");
	}
};

void Player::respawnPlayer() {
	if (pirate->hp > 0) return;
	ship->model.setTranslation(126, 0, -12);
	ship->model.rotate(2.1, Vector3(0, 1, 0));
	ship->scale(ship->scaleFactor);


	pirate->model.setTranslation(-100, FLOOR_HEIGHT * 3, -20);
	pirate->model.rotate(1.5, Vector3(0, 1, 0));
	pirate->scale(pirate->scaleFactor);

	pirate->hp = MAX_PLAYER_HP;

	Scene::world->currentIsle = Scene::world->startingIsle;
	AudioManager::audio->stop(Scene::world->currentMusic);
	Scene::world->currentMusic = AudioManager::audio->playloop("data/music/Island.wav");
}

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
void Player::initiateAttack() {
	pirate->hasHitSomeone = false;
	pirate->attacking = true;
	pirate->anim_time = 0;
	pirate->currAnimation = pirate->playerAnimations[3];
}

void Player::attack(float dt) {
	if (pirate->attacking) {
		if (pirate->anim_time < pirate->currAnimation->duration) pirate->anim_time += dt;
		else { 
			pirate->attacking = false;
			pirate->currAnimation = pirate->playerAnimations[0]; 
		}
		if (!pirate->hasHitSomeone && pirate->anim_time > 0.7 && pirate->anim_time < 0.8 && hitEnemy()) {
			pirate->hasHitSomeone = true;
		}
	}
}

bool Player::hitEnemy() {
	for (Skeli* enemy : Scene::world->currentIsle->enemies) {
		if (!enemy->alive) continue;
		Vector3 hitCenter = pirate->model * Vector3(0, 0, -1) + Vector3(0, 1.5, 0);
		Vector3 coll;
		Vector3 collnorm;

		if (!enemy->mesh->testSphereCollision(enemy->model, hitCenter, 2 / enemy->scaleFactor, coll, collnorm)) continue;

		Scene::world->currentIsle->activeEnemy = enemy;
		enemy->hp -= 1;

		return true;
	}
	return false;
}

void Player::initiateDodge() {
	pirate->dodging = true;
	pirate->anim_time = 0.2;
	pirate->currAnimation = pirate->playerAnimations[4];
}

void Player::dodge(float dt) {
	if (pirate->dodging) {
		if (pirate->anim_time < 1) pirate->anim_time += dt;
		else {
			pirate->dodging = false;
			pirate->currAnimation = pirate->playerAnimations[0];
		}
	}
}

void Player::die() {
	//after the dying animation
	respawnPlayer();
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
void Isle::createStuff() {
	std::string theme;
	bool palms = true;
	switch (this->type) {
	case 1: theme = "bushes"; palms = false; break;
	case 2: theme = "grass"; break;
	case 3: theme = "grass"; break;
	case 4: theme = "redFlowers"; palms = false; break;
	case 5: theme = "yellowFlowers"; break;
	case 6: theme = "yellowFlowers"; palms = false; break;
	default: theme = "grass";
	}

	int stuffLeft = 200;
	while (stuffLeft > 0) {
		Vector3 pos = getValidPosition();
		EntityMesh* stuff = new EntityMesh();
		this->noCollisionableThings.push_back(stuff);
		stuff->model.setIdentity();
		stuff->scale(75);
		stuff->model.rotate(random(6.28), Vector3(0, 1, 0));
		stuff->model.translateGlobal(pos.x, pos.y, pos.z);
		char variation = rand() % 2 + 1;
		stuff->loadMeshAndTexture(("data/decoration/noColli/" + theme + "/" + std::to_string(variation) + ".obj").c_str(), 
								("data/decoration/noColli/" + theme + "/" + std::to_string(variation) + ".tga").c_str());
		stuffLeft -= 1;
	}

	if (!palms) return;
	stuffLeft = 20;
	while (stuffLeft > 0) {
		Vector3 pos = getValidPosition();
		EntityMesh* stuff = new EntityMesh();
		this->collisionableThings.push_back(stuff);
		stuff->model.setIdentity();
		stuff->scale(120);
		stuff->model.rotate(random(6.28), Vector3(0, 1, 0));
		stuff->model.translateGlobal(pos.x, pos.y, pos.z);
		char variation = rand() % 2 + 1;
		stuff->loadMeshAndTexture(("data/decoration/colli/palm/" + std::to_string(variation) + ".obj").c_str(),
								("data/decoration/colli/palm/" + std::to_string(variation) + ".tga").c_str());
		stuffLeft -= 1;
	}

}

void Isle::createEnemies(int n) {
	int enemiesLeft = n;
	while (enemiesLeft > 0) {
		Skeli* enemy = new Skeli();
		Vector3 pos = getValidPosition();
		this->enemies.push_back(enemy);
		enemy->model.setIdentity();
		enemy->scale(2);
		enemy->model.translateGlobal(pos.x, pos.y, pos.z);
		enemy->loadMeshAndTexture("data/models/skeli/skeli.mesh", "data/models/skeli/color-atlas-new.tga");
		enemy->attacking = false;
		enemy->dodging = false;
		enemy->dying = false;
		enemy->attackTimer = 3;
		enemiesLeft -= 1;
	}
}

Vector3 Isle::getValidPosition() {
	bool validPos = false;
	Vector3 isleCenter = this->getPosition();
	Vector3 position;
	while (!validPos) {
		position = Vector3(random(APROX_ISLE_SIZE, isleCenter.x - APROX_ISLE_SIZE / 2), 1,
			random(APROX_ISLE_SIZE, isleCenter.z - APROX_ISLE_SIZE / 2));
		//supose it is valid
		validPos = true;
		//in isle and not colliding
		Vector3 coll;
		Vector3 collnorm;
		float padding = 8;
		if (!isAboveIsle(position, padding) ||
			this->mesh->testSphereCollision(this->model, position + Vector3(0, 1.5, 0), 1.4 / this->scaleFactor, coll, collnorm) ||
			this->mesh->testRayCollision(this->model, position + Vector3(0, 1.5, 0), Vector3(0, 1, 0), coll, collnorm, 100)) {
			validPos = false;
			continue;
		}
	}
	return position;
}

bool Isle::isAboveIsle(Vector3 pos) {
	Vector3 coll;
	Vector3 collnorm;
	return mesh->testRayCollision(model, pos, Vector3(0, -1, 0), coll, collnorm,0.5);
}

bool Isle::isAboveIsle(Vector3 pos, float padding) {
	return (isAboveIsle(pos) &&
		isAboveIsle(pos + Vector3(1, 0, 0) * padding) &&
		isAboveIsle(pos + Vector3(-1, 0, 0) * padding) &&
		isAboveIsle(pos + Vector3(0, 0, 1) * padding) &&
		isAboveIsle(pos + Vector3(0, 0, -1) * padding) &&
		isAboveIsle(pos + Vector3(0.71, 0, 0.71) * padding) &&
		isAboveIsle(pos + Vector3(-0.71, 0, 0.71) * padding) &&
		isAboveIsle(pos + Vector3(0.71, 0, -0.71) * padding) &&
		isAboveIsle(pos + Vector3(-0.71, 0, -0.71) * padding));
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
		isle->createStuff();
		isle->createEnemies(5);
		isle->enemiesLeft = true;
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
		if (position.length() < DIST_BTW_ISLES / 2.0) {
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

void Isle::updateEnemies(float dt) {
	for (Skeli* enemy : enemies) {
		if (!enemy->alive) continue;
		//kill enemy if 0 hp
		if (enemy->hp <= 0 && !enemy->dying) {
			if (activeEnemy == enemy) activeEnemy = NULL;
			enemy->initiateDie();
			continue;
		}
		enemy->die(dt);
		//MOVEMENT
		//create active enemy
		if (!enemy->attacking) {
			if (activeEnemy == NULL && enemy->isNearPlayer(9))
			{
				activeEnemy = enemy;
				enemy->followPlayer(dt);
				enemy->moving = false;
			}
			//get active enemy closer
			else if (activeEnemy == enemy && enemy->isNearPlayer(4))
			{
				enemy->followPlayer(dt);
				enemy->moving = false;
			}
			//allow active enemy go closer
			else if (activeEnemy == enemy && enemy->isNearPlayer(30))
			{
				enemy->increaseVelocity(dt);
				enemy->followPlayer(dt);
				enemy->moving = true;
			}
			//make other enemies wait
			else if (enemy->isNearPlayer(8))
			{
				enemy->followPlayer(dt);
				enemy->moving = false;
			}
			//make enemies get closer
			else if (enemy->isNearPlayer(30))
			{
				enemy->increaseVelocity(dt);
				enemy->followPlayer(dt);
				enemy->moving = true;
			}
			//too far away
			else {
				enemy->moving = false;
				if (activeEnemy == enemy) {
					activeEnemy = NULL;
				}
			}
		}


		//COMBAT
		if (activeEnemy == enemy) {
			if (enemy->isNearPlayer(5)) {
				enemy->attack(dt);
				if (enemy->attackTimer > 0) enemy->attackTimer -= dt;
				else
				{
					enemy->initiateAttack();
					enemy->attackTimer = 3;
				}
			}
			else enemy->attacking = false;
		}


		//Update animations
		if (!enemy->attacking && enemy->alive) {
			if (!enemy->moving) {
				enemy->currAnimation = enemy->skeliAnimations[0];
			}
			else {
				enemy->currAnimation = enemy->skeliAnimations[1];
			}
		}
		//else {
		//	enemy->currAnimation = enemy->skeliAnimations[3];
		//}



	}
};

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
Humanoid::Humanoid() {
	shader = Shader::Get("data/shaders/skinning.vs", "data/shaders/texture_phong.fs");
	color = Vector4(1, 1, 1, 1);
	currentVelocity = 0;
	maxVelocity = 2.5;
	currAnimation = skeliAnimations[0];
	currAnimation->assignTime(Game::instance->time);
}


void Humanoid::movePlayer(float dt) {
	
	Vector3 dir = Vector3();
	int anim = 0;
	if ((Input::gamepads[0].direction & PAD_UP && Input::gamepads[0].direction & PAD_LEFT) || (Input::isKeyPressed(SDL_SCANCODE_W) && Input::isKeyPressed(SDL_SCANCODE_A))) {
		/*model.translate(0, 0, -dt * currentVelocity * 0.6);
		model.translate(-dt * currentVelocity * 0.6, 0, 0);*/
		dir = Vector3(-0.6, 0, -0.6);
		increaseVelocity(dt);
		anim = 1;
	}
	else if ((Input::gamepads[0].direction & PAD_DOWN && Input::gamepads[0].direction & PAD_LEFT) || (Input::isKeyPressed(SDL_SCANCODE_S) && Input::isKeyPressed(SDL_SCANCODE_A))) {
		/*model.translate(0, 0, dt * currentVelocity * 0.3);
		model.translate(-dt * currentVelocity * 0.3, 0, 0);*/
		dir = Vector3(-0.3, 0, 0.3);
		increaseVelocity(dt);
		anim = 2;
	}
	else if ((Input::gamepads[0].direction & PAD_UP && Input::gamepads[0].direction & PAD_RIGHT) || (Input::isKeyPressed(SDL_SCANCODE_W) && Input::isKeyPressed(SDL_SCANCODE_D))) {
		/*model.translate(0, 0, -dt * currentVelocity * 0.6);
		model.translate(dt * currentVelocity * 0.6, 0, 0);*/
		dir = Vector3(0.6, 0, -0.6);
		increaseVelocity(dt);
		anim = 1;
	}
	else if ((Input::gamepads[0].direction & PAD_DOWN && Input::gamepads[0].direction & PAD_RIGHT) || (Input::isKeyPressed(SDL_SCANCODE_S) && Input::isKeyPressed(SDL_SCANCODE_D))) {
		/*model.translate(0, 0, dt * currentVelocity * 0.3);
		model.translate(dt * currentVelocity * 0.3, 0, 0);*/
		dir = Vector3(0.3, 0, 0.3);
		increaseVelocity(dt);
		anim = 2;
	}
	else if(Input::gamepads[0].direction & PAD_UP || (Input::isKeyPressed(SDL_SCANCODE_W))){
		//model.translate(0, 0, -dt * currentVelocity); 
		dir = Vector3(0, 0, -1);
		increaseVelocity(dt);
		anim = 1;
	}
	else if (Input::gamepads[0].direction & PAD_DOWN || (Input::isKeyPressed(SDL_SCANCODE_S))) {
		//model.translate(0, 0, dt * currentVelocity * 0.5);
		dir = Vector3(0, 0, 0.5);
		increaseVelocity(dt);
		anim = 2;
	}
	else if (Input::gamepads[0].direction & PAD_LEFT || (Input::isKeyPressed(SDL_SCANCODE_A))) {
		//model.translate(-dt * currentVelocity * 0.75, 0, 0);
		dir = Vector3(-0.75, 0, 0);
		increaseVelocity(dt);
		anim = 2;
	}
	else if (Input::gamepads[0].direction & PAD_RIGHT || (Input::isKeyPressed(SDL_SCANCODE_D))) {
		//model.translate(dt * currentVelocity * 0.75, 0, 0);
		dir = Vector3(0.75, 0, 0);
		increaseVelocity(dt);
		anim = 2;
	}
	else {
		anim = 0;
	}
	if (currentVelocity == 0) return;

	currAnimation = playerAnimations[anim];
		
	model.translate(dt * currentVelocity * dir.x, 0, dt * currentVelocity * dir.z);

	currentVelocity = clamp(currentVelocity - dt * 5, 0, maxVelocity);
	Isle* isle = Scene::world->currentIsle;
	Vector3 targetCenter = model.getTranslation() + Vector3(0, 1.5, 0);
	Vector3 coll;
	Vector3 collnorm;

	//Check if in the isle
	if (!isle->isAboveIsle(this) && isle->mesh->testSphereCollision(isle->model, targetCenter, 2 / isle->scaleFactor, coll, collnorm)) {
		//std::cout << "Out" << std::endl;
		Vector3 push_in = normalize(Vector3(coll.x - targetCenter.x, 0.001, coll.z - targetCenter.z)) * dt * currentVelocity * scaleFactor;
		model.translateGlobal(push_in.x, 0, push_in.z);
	}
		//model.translate(-dt * currentVelocity * dir.x, 0, -dt * currentVelocity * dir.z); 

	//Check collisions with isle
	if (isle->mesh->testSphereCollision(isle->model, targetCenter, 1.4 / isle->scaleFactor, coll, collnorm)) {
		Vector3 push_away = normalize(Vector3(coll.x - targetCenter.x, 0.001, coll.z - targetCenter.z)) * dt * currentVelocity * scaleFactor;
		model.translateGlobal(-push_away.x, 0, -push_away.z);
	}
	//Check collisino with palms
	for (EntityMesh* palm : isle->collisionableThings) {
		if (palm->mesh->testSphereCollision(palm->model, targetCenter, 1.4 / palm->scaleFactor, coll, collnorm)) {
			Vector3 push_away = normalize(Vector3(coll.x - targetCenter.x, 0.001, coll.z - targetCenter.z)) * dt * currentVelocity * scaleFactor;
			model.translateGlobal(-push_away.x, 0, -push_away.z);
		}
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

void Skeli::followPlayer(float dt) {
	Vector3 playerPos = Scene::world->player->pirate->getPosition();
	Vector3 enemyPos = this->getPosition();
	Vector3 dir = Vector3(enemyPos.x - playerPos.x, 0, enemyPos.z - playerPos.z);
	if (dir.x == 0 && dir.z == 0) return;
	//assert((dir.x != 0 || dir.z != 0) && "Enemy is already at player position");
	this->model.setFrontAndOrthonormalize(dir);
	this->model.setUpAndOrthonormalize(Vector3(0,1,0));
	this->scale(this->scaleFactor);

	model.translate(0, 0, -dt * currentVelocity);
	currentVelocity = clamp(currentVelocity - dt * 5, 0, maxVelocity);
}

bool Skeli::isNearPlayer(int radius) {
	Vector3 playerPos = Scene::world->player->pirate->getPosition();
	Vector3 enemyPos = this->getPosition();
	return ((playerPos - enemyPos).length() < radius);
}

void Skeli::initiateAttack() {
	hasHitSomeone = false;
	attacking = true;
	anim_time = 0;
	currAnimation = skeliAnimations[2];
}

void Skeli::attack(float dt) {
	if (attacking) {
		if (anim_time < currAnimation->duration) anim_time += dt;
		else {
			attacking = false;
			currAnimation = skeliAnimations[0];
		}

		if (!hasHitSomeone && anim_time > 0.9 && anim_time < 1 && hitPlayer()) {
			hasHitSomeone = true;
		}
	}
}

bool Skeli::hitPlayer() {
	Humanoid* player = Scene::world->player->pirate;
	if (player->dodging) return false;
	Vector3 hitCenter = model * Vector3(0, 0, -1) + Vector3(0, 1.5, 0);
	Vector3 coll;
	Vector3 collnorm;

	if (!player->mesh->testSphereCollision(player->model, hitCenter, 2 / player->scaleFactor, coll, collnorm)) return false;

	player->hp -= 1;

	return true;
}
void Skeli::initiateDie()
{
	dying = true;
	anim_time = 0;
	currAnimation = skeliAnimations[3];
	Scene::world->player->points += 1;
}

void Skeli::die(float dt) {
	//after the dying animation
	if (dying) {
		if (anim_time < currAnimation->duration - 0.1) anim_time += dt;
		else {
			alive = false;
			//currAnimation = skeliAnimations[0];
		}
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

	//mesh->renderAnimated(GL_TRIANGLES, idle->skeleton);

	//disable the shader after finishing rendering
	shader->disable();
}
//----------------------------------------Sword----------------------------------------//
void Sword::render()
{
	//Check Frustum
	//if(!camera->)
	//get the last camera that was activated
	Camera* camera = Camera::current;
	Matrix44 model = this->model;

	//enable shader and pass uniforms
	shader->enable();
	shader->setUniform("u_color", color);
	//model.rotate(180.0f * DEG2RAD, Vector3(0, 1, 0));
	shader->setUniform("u_model", model);
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_texture", texture, 0);
	shader->setUniform("u_time", Game::instance->time);
	shader->setUniform("u_eye", camera->eye);

	//render the mesh using the shader
	mesh->render(GL_TRIANGLES);

	//mesh->renderAnimated(GL_TRIANGLES, idle->skeleton);

	//disable the shader after finishing rendering
	shader->disable();

}//----------------------------------------Ship Render----------------------------------------//
void Ship::render()
{
	//Check Frustum
	//if(!camera->)
	//get the last camera that was activated
	Camera* camera = Camera::current;
	Matrix44 model = this->model;

	//enable shader and pass uniforms
	shader->enable();
	shader->setUniform("u_color", color);
	Vector3 p = model.getTranslation();
	//0.1 * sin(Game::instance->time)
	model.translate(0.2 * sin(Game::instance->time+PI), -0.2 * sin(Game::instance->time) - 0.5, 0.2 * sin(Game::instance->time));
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


//----------------------------------------Humanoid Render----------------------------------------//
void Humanoid::render()
{
	//Check Frustum
	//if(!camera->)
	//get the last camera that was activated
	Camera* camera = Camera::current;
	Matrix44 model = this->model;

	//enable shader and pass uniforms
	shader->enable();
	shader->setUniform("u_color", color);
	model.rotate(180.0f * DEG2RAD, Vector3(0, 1, 0));
	shader->setUniform("u_model", model);
	shader->setUniform("u_viewprojection", camera->viewprojection_matrix);
	shader->setUniform("u_texture", texture, 0);
	shader->setUniform("u_time", Game::instance->time);
	shader->setUniform("u_eye", camera->eye);

	//render the mesh using the shader
	//mesh->render(GL_TRIANGLES);

	if(attacking | dodging)currAnimation->assignTime(anim_time);
	else currAnimation->assignTime(Game::instance->time);

	mesh->renderAnimated(GL_TRIANGLES, &currAnimation->skeleton);

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
	shader->setUniform("u_texture_tiling", tiles);
	mesh->render(GL_TRIANGLES);
	glDisable(GL_BLEND);
	glDepthMask(true);
	shader->disable();
}



//----------------------------------------AudioManager----------------------------------------//

AudioManager::AudioManager() {
	//Inicializamos BASS al arrancar el juego (id_del_device, muestras por segundo, ...)
	if (BASS_Init(-1, 44100, 0, 0, NULL) == false) //-1 significa usar el por defecto del sistema operativo
	{
		//error abriendo la tarjeta de sonido...
	}
};

HCHANNEL AudioManager::play(const char* filename) {

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
		hSample = *it->second;
		hSampleChannel = BASS_SampleGetChannel(hSample, false);
		BASS_ChannelPlay(hSampleChannel, true);
		return hSampleChannel;
	}
	//else
	//{
	//	//load it

	//	HSAMPLE hSample = BASS_SampleLoad(false, filename, 0, 0, 20, 0);


	//	hSampleChannel = BASS_SampleGetChannel(hSample, false);
	//	sSamplesLoaded[filename] = &hSample;
	//	BASS_ChannelPlay(hSampleChannel, true);

	//	return hSample;
	//};
}
HCHANNEL AudioManager::playloop(const char* filename) {
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
		hSample = *(it->second);
		hSampleChannel = BASS_SampleGetChannel(hSample, false);
		BASS_ChannelPlay(hSampleChannel, true);
		return hSampleChannel;
	}
	//else
	//{
	//    //load it

	//    HSAMPLE hSample = BASS_SampleLoad(false, filename, 0, 0, 20, BASS_SAMPLE_LOOP);


	//    hSampleChannel = BASS_SampleGetChannel(hSample, false);
	//    sSamplesLoaded[filename] = &hSample;
	//    BASS_ChannelPlay(hSampleChannel, true);

	//    return hSample;
	//};

}

void AudioManager::stop(HCHANNEL hSample) {
	BASS_ChannelPause(hSample);
}

void AudioManager::loadSamples() {
	
	hSample1 = BASS_SampleLoad(false, "data/music/Battle.wav", 0, 0, 20, BASS_SAMPLE_LOOP);
	sSamplesLoaded["data/music/Battle.wav"] = &hSample1;
	hSample2 = BASS_SampleLoad(false, "data/music/Island.wav", 0, 0, 20, BASS_SAMPLE_LOOP);
	sSamplesLoaded["data/music/Island.wav"] = &hSample2;
	hSample3 = BASS_SampleLoad(false, "data/music/Onepiece.wav", 0, 0, 20, BASS_SAMPLE_LOOP);
	sSamplesLoaded["data/music/Onepiece.wav"] = &hSample3;
	hSample4 = BASS_SampleLoad(false, "data/sounds/boneCrack1.wav", 0, 0, 20, 0);
	sSamplesLoaded["data/sounds/boneCrack1.wav"] = &hSample4;
	hSample5 = BASS_SampleLoad(false, "data/sounds/boneCrack2.wav", 0, 0, 20, 0);
	sSamplesLoaded["data/sounds/boneCrack2.wav"] = &hSample5;
	hSample6 = BASS_SampleLoad(false, "data/sounds/dodge.wav", 0, 0, 20, 0);
	sSamplesLoaded["data/sounds/dodge.wav"] = &hSample6;
	hSample7 = BASS_SampleLoad(false, "data/sounds/hit.wav", 0, 0, 20, 0);
	sSamplesLoaded["data/sounds/hit.wav"] = &hSample7;
	hSample8 = BASS_SampleLoad(false, "data/sounds/swing.wav", 0, 0, 20, 0);
	sSamplesLoaded["data/sounds/swing.wav"] = &hSample8;
	hSample9 = BASS_SampleLoad(false, "data/sounds/sword.wav", 0, 0, 20, 0);
	sSamplesLoaded["data/sounds/sword.wav"] = &hSample9;
}