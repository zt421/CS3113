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

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

SDL_Window* displayWindow;

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



int main(int argc, char *argv[])
{
	//Zami Talukder
	//HW2 Intro to Game Programming

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	bool done = false;
	//attempt
	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	//paddles and win textures
	GLuint paddleTexture = LoadTexture(RESOURCE_FOLDER"redPaddle.png");
	GLuint leftWin = LoadTexture(RESOURCE_FOLDER"leftWin.png");
	GLuint rightWin = LoadTexture(RESOURCE_FOLDER"rightWin.png");

	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);

	//initialized many variables
	float p1PaddlePosition = 0;
	float p2PaddlePosition = 0;

	float ballX = 0;
	float ballY = 0;
	bool goingDown = true;
	bool goingLeft = true;
	int whoWon = 0;

	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;
	float p1X = -3;
	float p2X = 3;

	while (!done) {

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			//checks for keyboard inputs. UP, DOWN for player 1, W and S for player 2
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_UP && p1PaddlePosition <= 1.4)
				{
					p1PaddlePosition += 10 * elapsed;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_DOWN && p1PaddlePosition>= -1.4)
				{
					p1PaddlePosition -= 10 * elapsed;
				}

				if (event.key.keysym.scancode == SDL_SCANCODE_W && p2PaddlePosition <= 1.4)
				{
					p2PaddlePosition += 10 * elapsed;
				}
				if (event.key.keysym.scancode == SDL_SCANCODE_S && p2PaddlePosition >= -1.4)
				{
					p2PaddlePosition -= 10 * elapsed;
				}

			}
		}

		///FOR MOVING BALL Y COORDINATE
		if (ballY >= 2)
		{
			goingDown = true;
		}
		else if (ballY <= -2)
		{
			goingDown = false;
		}

		if (goingDown)
		{
			ballY-= 4 * elapsed;
		}
		else if (!goingDown)
		{
			ballY += 4 * elapsed;
		}

		//FOR MOVING BALL X COORDINATE
		if ( !(ballX+.15 <= p2X-.15 || ballX-.15 >= p2X+.15 || ballY+.15 <= p2PaddlePosition-.5 || ballY-.15 >= p2PaddlePosition+.5))
		{
			goingLeft = true;
		}
		else if ( !(ballX+.15 <= p1X-.15 || ballX-.15 >= p1X+.15 || ballY+.15 <= p1PaddlePosition-.5 || ballY-.15 >= p1PaddlePosition+.5))
		{
			goingLeft = false;
		}
		else if (ballX >= 3.5) //for when player fails to hit ball
		{
			whoWon = 1;
			
		}
		else if (ballX <= -3.5)
		{
			whoWon = 2;
		}

		if (goingLeft)   
		{
			ballX -= 4 * elapsed;
		}
		else if (!goingLeft)
		{
			ballX += 4 * elapsed;
		}

		




		glClear(GL_COLOR_BUFFER_BIT);

		
		

		
		/////player 1
		modelMatrix.identity();
		modelMatrix.Translate(p1X, p1PaddlePosition, 0);
		
		program.setModelMatrix(modelMatrix);
		program.setProjectionMatrix(projectionMatrix);
		program.setViewMatrix(viewMatrix);

		glBindTexture(GL_TEXTURE_2D, paddleTexture);

		
		float vertices[] = { -0.15, -0.5, 0.15, -0.5, 0.15, 0.5, -0.15, -0.5, 0.15, 0.5, -0.15, 0.5 };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);



		////////////////////////////////////////////////
		//player 2

		modelMatrix.identity();
		modelMatrix.Translate(p2X, p2PaddlePosition, 0);

		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, paddleTexture);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);

		////////////////////////////////////////////////
		//ball
		modelMatrix.identity();
		modelMatrix.Translate(ballX, ballY, 0);

		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, paddleTexture);

		float vertices2[] = { -0.15, -0.15, 0.15, -0.15, 0.15, 0.15, -0.15, -0.15, 0.15, 0.15, -0.15, 0.15 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		////////////////////////////////////////////////
		//p1 won
		float vertices3[] = { -4, -2, 4, -2, 4, 2, -4, -2, 4, 2, -4, 2 };
		if (whoWon == 1)
		{
			modelMatrix.identity();
			program.setModelMatrix(modelMatrix);

			glBindTexture(GL_TEXTURE_2D, leftWin);


			

			glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
			glEnableVertexAttribArray(program.positionAttribute);

			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		///p2 won
		else if (whoWon == 2)
		{
			modelMatrix.identity();
			program.setModelMatrix(modelMatrix);

			glBindTexture(GL_TEXTURE_2D, rightWin);



			glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices3);
			glEnableVertexAttribArray(program.positionAttribute);

			glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
			glEnableVertexAttribArray(program.texCoordAttribute);

			glDrawArrays(GL_TRIANGLES, 0, 6);

		}


		/////////////////////////////////////////////////

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		SDL_GL_SwapWindow(displayWindow);
	}



	SDL_Quit();
	return 0;
}
