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
	texture1 = new Texture();
	texture1->load("data/texture.tga");

	// example of loading Mesh from Mesh Manager
	mesh1 = Mesh::Get("data/box.ASE");

	// example of shader loading using the shaders manager
	shader1 = Shader::Get("data/shaders/basic.vs", "data/shaders/texture.fs");
}






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

	//create model matrix for cube
	Matrix44 m;
	m.rotate(angle1 * DEG2RAD, Vector3(0, 1, 0));

	if (shader1)
	{
		//enable shader
		shader1->enable();

		//upload uniforms
		shader1->setUniform("u_color", Vector4(1, 1, 1, 1));
		shader1->setUniform("u_viewprojection", Game::instance->camera->viewprojection_matrix);
		shader1->setUniform("u_texture", texture1, 0);
		shader1->setUniform("u_model", m);
		shader1->setUniform("u_time", Game::instance->time);

		//do the draw call
		mesh1->render(GL_TRIANGLES);

		//disable shader
		shader1->disable();
	}

	//Draw the floor grid
	drawGrid();

	//render the FPS, Draw Calls, etc
	drawText(2, 2, getGPUStats(), Vector3(1, 1, 1), 2);

	

}

void PlayStage::update(double dt) {
	

}