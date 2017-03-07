#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"

#include <Windows.h>
#include <vector>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include <cmath>

SDL_Window* displayWindow;

struct Vector3 {
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
};

struct TextCoords {
	TextCoords(float left, float right, float top, float bot)
		: left(left), right(right), top(top), bot(bot) {}
	float left;
	float right;
	float bot;
	float top;
};


class Entity {
public:
	Entity(GLuint textureID = 0, float size = 1, float aspectRatio = 1, Vector3 position = Vector3(0,0,0),
		TextCoords text = TextCoords(0.0,1.0,0.0,1.0) ): textureID(textureID), size(size), aspectRatio(aspectRatio),
		position(position), textCoords(text) {
		halfWidth = size*aspectRatio*.5;
		halfHeight = size*.5;
	}

	//draws the entity with the given x,y,z coordinates and texture coordinates, and size of entity
	void Draw(ShaderProgram& program) {
		if (!visible) return;
		Matrix modelMatrix;

		modelMatrix.identity();
		modelMatrix.Translate(position.x, position.y, position.z);

		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, textureID);

		float vertices[] = { -halfWidth, -halfHeight, halfWidth, -halfHeight, halfWidth, halfHeight, 
			-halfWidth, -halfHeight, halfWidth, halfHeight, -halfWidth, halfHeight };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] =               
		{
			textCoords.left, textCoords.bot,
			textCoords.right, textCoords.bot,
			textCoords.right, textCoords.top,
			textCoords.left, textCoords.bot,
			textCoords.right, textCoords.top,
			textCoords.left, textCoords.top };
		    
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void update(float elapsed)
	{
		position.x += speed * cos(direction*3.141592/180) * elapsed;
		position.y += speed * sin(direction*3.141592/180) * elapsed;
	}

	bool checkBoxBoxCollision(const Entity& object)
	{
		if (!object.visible || !visible) return false;
		if (!(position.x + halfWidth <= object.position.x - object.halfWidth ||
			position.x - halfWidth >= object.position.x + object.halfWidth ||
			position.y + halfHeight <= object.position.y - object.halfHeight ||
			position.y - halfHeight >= object.position.y + object.halfHeight))
		{
			return true;
		}
		else
			return false;
	}

	Vector3 position;
	GLuint textureID;
	float halfWidth;
	float halfHeight;
	float speed = 0;
	float direction = 0;
	TextCoords textCoords;
	bool visible = true;
	float aspectRatio;
	float size;
};


//given from class
void DrawText1(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing, float x, float y) {
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
		});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);

	Matrix modelMatrix;

	modelMatrix.identity();
	modelMatrix.Translate(x, y, 0);

	program->setModelMatrix(modelMatrix);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.size()*6);
}





GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}
	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}


enum GameState {Start, Running, GameOver };

int main(int argc, char *argv[])
{
	//Zami Talukder
	//HW3 Intro to Game Programming

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	bool done = false;
	
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	Matrix viewMatrix;
	Matrix modelMatrix;
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	//initialized many variables



	GLuint spaceSheet = LoadTexture(RESOURCE_FOLDER"spaceinvaders.png"); //(106,159) (0-92,0-83)(0-92,83-159)(93-106,0-56)
	GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");  //16,16  512,512

	Entity player(spaceSheet, .6, 98.0/(159-84), Vector3(-1, -1.5, 0),  TextCoords(0.0, 98.0 / 106, 84.0 / 159, 1.0));

	Entity bullets[5];
	int bulletIndex = -1;
	for (int i = 0; i < 5; i++)
	{
		bullets[i] = Entity(spaceSheet, .2, (106-93)/56.0, Vector3(5, 5, 0), TextCoords(93.0 / 106, 1.0, 0.0, 56.0 / 159));
	}

	
	//makes a list of enemies
	Entity enemyList[3][6];
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			enemyList[i][j] = Entity(spaceSheet, .4, 92.0/83, Vector3(-2.5 + j, .5+i*.5, 1), TextCoords(0.0, 92.0 / 106, 0.0, 83.0 / 159));
			enemyList[i][j].speed = 1;
			enemyList[i][j].direction = 180;
		}
	}
	GameState gamestate = Start;

	glUseProgram(program.programID);


	float lastFrameTicks = 0.0f;
	float lastFired = 0; //for delaying gun use

	while (!done) {

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		lastFired += elapsed;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			if (gamestate == Start && event.key.keysym.scancode == SDL_SCANCODE_SPACE)
			{
				gamestate = Running;
			}
			if (gamestate == Running && event.key.keysym.scancode == SDL_SCANCODE_SPACE && lastFired >= 1)
			{
				lastFired = 0;
				bulletIndex++;
				if (bulletIndex >= 5)
					bulletIndex = 0;
				bullets[bulletIndex].visible = true;
				bullets[bulletIndex].position.x = player.position.x;
				bullets[bulletIndex].position.y = player.position.y + player.halfHeight;
				bullets[bulletIndex].speed = 2.0;
				bullets[bulletIndex].direction = 90;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);

		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		if (gamestate == Start)
		{
			DrawText1(&program, fontSheet, "Press Space to Start", .3, 0, -3, 0);
		}
		else if (gamestate == Running)
		{

			//checks for keyboard inputs.
			const Uint8 *keys = SDL_GetKeyboardState(NULL);

			if (keys[SDL_SCANCODE_LEFT] && player.position.x >= -3)
			{
				player.position.x -= 2 * elapsed;
			}
			else if (keys[SDL_SCANCODE_RIGHT] && player.position.x <= 3)
			{
				player.position.x += 2 * elapsed;
			}

			player.Draw(program);
			//updates and draws every bullet
			for (int i = 0; i < 5; i++)
			{
				bullets[i].update(elapsed);
				bullets[i].Draw(program);
			}

			//updates and draws top row of enemies

			bool stillAlive = false; //used to check if game ended
			int moveLeft = 0;  //tells when to change direction and move enemies down. 0-do nothing, 1- move left, 2-move right

			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 6; j++)
				{

					if (enemyList[i][j].visible == true) //checks if all enemies are dead
						stillAlive = true;

					if (enemyList[0][0].position.x <= -3) //sets flag for when to move right
						moveLeft = 2;

					if (enemyList[0][5].position.x >= 3) //sets flag for when to move left
						moveLeft = 1;

					if (moveLeft == 1) //ships reached leftmost side and must move right
					{
						enemyList[i][j].direction = 180;
						enemyList[i][j].position.y -= .02;
					}
					if (moveLeft == 2) //must move left
					{
						enemyList[i][j].direction = 0;
						enemyList[i][j].position.y -= .02;
					}

					for (int p = 0; p < 6; p++) //checks box-box collision between bullets and ships
					{
						if (enemyList[i][j].checkBoxBoxCollision(bullets[p]))
						{
							enemyList[i][j].visible = false;
							bullets[p].visible = false;
						}
					}

					if (enemyList[i][j].visible && enemyList[i][j].position.y <= 0)
						gamestate = GameOver;



				}
			}
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 6; j++)
				{
					enemyList[i][j].update(elapsed);
					enemyList[i][j].Draw(program);
				}
			}


			if (!stillAlive)
				gamestate = GameOver;
		}
		else if (gamestate == GameOver)
		{
			DrawText1(&program, fontSheet, "Game Over", .4, 0, -1, 0);
		}

		/////////////////////////////////////////////////

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}



	SDL_Quit();
	return 0;
}
