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



Scene::Scene() {
	//load one texture without using the Texture Manager (Texture::Get would use the manager)

	cube = EntityMesh();
	Matrix44 m;
	m.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	cube.model = m;
	cube.texture = new Texture();
	cube.texture->load("data/texture.tga");
	cube.mesh = Mesh::Get("data/box.ASE");
	cube.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	cube.color = Vector4(1,1,1,1);
	
	ship = EntityMesh();
	Matrix44 m2;
	m2.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));
	ship.model = m2;
	ship.texture = new Texture();
	ship.texture->load("data/texture.tga");
	ship.mesh = Mesh::Get("data/boat_large.obj");
	ship.shader = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
	ship.color = Vector4(1, 1, 1, 1);

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


	Scene::world->ship.render();

	//Draw the floor grid
	drawGrid();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

	

}

void PlayStage::update(double dt) {
	

}

//ENTITY METHODS
void EntityMesh::render()
{
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
