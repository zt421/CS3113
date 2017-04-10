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
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;






SDL_Window* displayWindow;





class Vector3 {
public:
	Vector3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
	Vector3 operator*(const Matrix& matrix) {
		return Vector3(matrix.m[0][0] * x + matrix.m[1][0] * y + matrix.m[2][0] * z + matrix.m[3][0],
			matrix.m[0][1] * x + matrix.m[1][1] * y + matrix.m[2][1] * z + matrix.m[3][1],
			matrix.m[0][2] * x + matrix.m[1][2] * y + matrix.m[2][2] * z + matrix.m[3][2]);
	}

	float length() const {
		return sqrt((x*x) + (y*y));
	}
};

struct TextCoords {
	TextCoords(float left=0, float right=1, float top=0, float bot=1)
		: left(left), right(right), top(top), bot(bot) {}
	float left;
	float right;
	float bot;
	float top;
};

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t*v1;
}



enum EntityType {ENTITIY_PLAYER, ENTITY_ENEMY, ENTITY_COIN};
class Entity {
public:
	Entity(GLuint textureID = 0, float size = 1, float aspectRatio = 1, Vector3 position = Vector3(0,0,0),
		TextCoords text = TextCoords(0.0,1.0,0.0,1.0), EntityType type = ENTITIY_PLAYER ): textureID(textureID), size(size), aspectRatio(aspectRatio),
		position(position), textCoords(text), type(type) {
		halfLengths = Vector3(size*aspectRatio*.5, size*.5, 0);
		acceleration = Vector3(0, 0, 0);
		velocity = Vector3(0, 0, 0);
	}

	Entity(EntityType type, Vector3 position) : type(type), position(position) {}

	EntityType type;
	Vector3 position;
	Vector3 scale = Vector3(1,1,1);
	float rotation=0;
	Vector3 acceleration;
	Vector3 velocity;
	Vector3 halfLengths;
	Vector3 friction;
	GLuint textureID;
	TextCoords textCoords;
	vector<Entity*>* entities = nullptr;
	vector<Vector3> worldVertices;

	bool isStatic = false;
	bool visible = true;
	float aspectRatio;  //x/y
	float size;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
	

	//draws the entity with the given x,y,z coordinates and texture coordinates, and size of entity
	void Draw(ShaderProgram& program) {
		if (!visible) return;
		Matrix modelMatrix;

		modelMatrix.identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Rotate(rotation);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		worldVertices.clear();
		worldVertices.push_back(Vector3(-halfLengths.x, -halfLengths.y) * modelMatrix);
		worldVertices.push_back(Vector3(halfLengths.x, -halfLengths.y) * modelMatrix);
		worldVertices.push_back(Vector3(halfLengths.x, halfLengths.y) * modelMatrix);
		worldVertices.push_back(Vector3(-halfLengths.x, halfLengths.y) * modelMatrix);

		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, textureID);

		float vertices[] = { -halfLengths.x, -halfLengths.y, halfLengths.x, -halfLengths.y, halfLengths.x, halfLengths.y,
			-halfLengths.x, -halfLengths.y, halfLengths.x, halfLengths.y, -halfLengths.x, halfLengths.y };
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

	//This method is used instead of draw when drawn with sprite sheets

	
	void update(float elapsed)
	{
		velocity.x += acceleration.x * elapsed;
		velocity.y += acceleration.y * elapsed;

		velocity.x = lerp(velocity.x, 0, elapsed * friction.x);
		position.x += velocity.x *  elapsed;
		Entity* colliding = checkBoxBoxCollision();
		moveAway('x', colliding);

		velocity.y = lerp(velocity.y, 0, elapsed * friction.y);
		position.y += velocity.y *  elapsed;
		colliding = checkBoxBoxCollision();
		moveAway('y', colliding);

		collidedTop = false;
		collidedBottom = false;
		collidedLeft = false;
		collidedRight = false;
	}


	//checks actual boxboxcollision
	Entity* checkBoxBoxCollision()
	{
		if (!visible) return nullptr;
		if (this->entities == nullptr) return nullptr;
		for (int i = 0; i < this->entities->size(); i++)
		{
			if ((*(this->entities))[i]->type != ENTITIY_PLAYER) {
				if (!(*(this->entities))[i]->visible || !visible) continue;
				if (!(position.x + halfLengths.x <= (*(this->entities))[i]->position.x - (*(this->entities))[i]->halfLengths.x ||
					position.x - halfLengths.x >= (*(this->entities))[i]->position.x + (*(this->entities))[i]->halfLengths.x ||
					position.y + halfLengths.y <= (*(this->entities))[i]->position.y - (*(this->entities))[i]->halfLengths.y ||
					position.y - halfLengths.y >= (*(this->entities))[i]->position.y + (*(this->entities))[i]->halfLengths.y))
				{
					(*(this->entities))[i]->visible = false;
					return (*(this->entities))[i];
				}
			}
		}
		return nullptr;
	}

	//moves entity away from object depending on dimension passed in
	//'x' for up/down dimension, anything for y, does not check z
	void moveAway(char dimension, Entity* object)
	{
		if (object == nullptr) return;
		Vector3 penetration = Vector3(0, 0, 0);
		
		penetration.x = fabs(fabs(position.x - object->position.x) - halfLengths.x - object->halfLengths.x);
		penetration.y = fabs(fabs(position.y - object->position.y) - halfLengths.y - object->halfLengths.y);

		if (dimension == 'x') {
			if (object->position.x < position.x)
				position.x += penetration.x + .00001;
			else
				position.x -= penetration.x + .00001;
		}
		else {
			if (object->position.y < position.y)
				position.y += penetration.y + .00001;
			else
				position.y -= penetration.y + .00001;
		}
		

	}
	
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
	glDrawArrays(GL_TRIANGLES, 0, text.size()/2);
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


bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector3> &points1, const std::vector<Vector3> &points2, Vector3 &penetration) {
	float normalX = -edgeY;
	float normalY = edgeX;
	float len = sqrtf(normalX*normalX + normalY*normalY);
	normalX /= len;
	normalY /= len;

	std::vector<float> e1Projected;
	std::vector<float> e2Projected;

	for (int i = 0; i < points1.size(); i++) {
		e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
	}
	for (int i = 0; i < points2.size(); i++) {
		e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
	}

	std::sort(e1Projected.begin(), e1Projected.end());
	std::sort(e2Projected.begin(), e2Projected.end());

	float e1Min = e1Projected[0];
	float e1Max = e1Projected[e1Projected.size() - 1];
	float e2Min = e2Projected[0];
	float e2Max = e2Projected[e2Projected.size() - 1];

	float e1Width = fabs(e1Max - e1Min);
	float e2Width = fabs(e2Max - e2Min);
	float e1Center = e1Min + (e1Width / 2.0);
	float e2Center = e2Min + (e2Width / 2.0);
	float dist = fabs(e1Center - e2Center);
	float p = dist - ((e1Width + e2Width) / 2.0);

	if (p >= 0) {
		return false;
	}

	float penetrationMin1 = e1Max - e2Min;
	float penetrationMin2 = e2Max - e1Min;

	float penetrationAmount = penetrationMin1;
	if (penetrationMin2 < penetrationAmount) {
		penetrationAmount = penetrationMin2;
	}

	penetration.x = normalX * penetrationAmount;
	penetration.y = normalY * penetrationAmount;

	return true;
}

bool penetrationSort(const Vector3 &p1, const Vector3 &p2) {
	return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector3> &e1Points, const std::vector<Vector3> &e2Points, Vector3 &penetration) {
	std::vector<Vector3> penetrations;
	if (e1Points.empty() || e2Points.empty()) return false;
	for (int i = 0; i < e1Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e1Points.size() - 1) {
			edgeX = e1Points[0].x - e1Points[i].x;
			edgeY = e1Points[0].y - e1Points[i].y;
		}
		else {
			edgeX = e1Points[i + 1].x - e1Points[i].x;
			edgeY = e1Points[i + 1].y - e1Points[i].y;
		}
		Vector3 penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}
	for (int i = 0; i < e2Points.size(); i++) {
		float edgeX, edgeY;

		if (i == e2Points.size() - 1) {
			edgeX = e2Points[0].x - e2Points[i].x;
			edgeY = e2Points[0].y - e2Points[i].y;
		}
		else {
			edgeX = e2Points[i + 1].x - e2Points[i].x;
			edgeY = e2Points[i + 1].y - e2Points[i].y;
		}
		Vector3 penetration;
		bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);

		if (!result) {
			return false;
		}
		penetrations.push_back(penetration);
	}

	std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
	penetration = penetrations[0];

	Vector3 e1Center;
	for (int i = 0; i < e1Points.size(); i++) {
		e1Center.x += e1Points[i].x;
		e1Center.y += e1Points[i].y;
	}
	e1Center.x /= (float)e1Points.size();
	e1Center.y /= (float)e1Points.size();

	Vector3 e2Center;
	for (int i = 0; i < e2Points.size(); i++) {
		e2Center.x += e2Points[i].x;
		e2Center.y += e2Points[i].y;
	}
	e2Center.x /= (float)e2Points.size();
	e2Center.y /= (float)e2Points.size();

	Vector3 ba;
	ba.x = e1Center.x - e2Center.x;
	ba.y = e1Center.y - e2Center.y;

	if ((penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
		penetration.x *= -1.0f;
		penetration.y *= -1.0f;
	}

	return true;
}


Entity player;
Entity object1;
Entity object2;

bool ProcessEvents(SDL_Event& event,float elapsed) {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			return true;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_UP)
		{
			player.position.y += 3 * elapsed;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)
		{
			player.position.y -= 3 * elapsed;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
		{
			player.position.x -= 3 * elapsed;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
		{
			player.position.x += 3 * elapsed;
		}
	}
	if (object1.position.x >= 3.5) {
		object1.position.x = 3.45;
		object1.velocity.x *= -1;
	}
	if (object1.position.x <= -3.5) {
		object1.position.x = -3.45;
		object1.velocity.x *= -1;
	}
	if (object1.position.y >= 1.9) {
		object1.position.y = 1.85;
		object1.velocity.y *= -1;
	}
	if (object1.position.y <= -1.9) {
		object1.position.y = -1.85;
		object1.velocity.y *= -1;
	}

	if (object2.position.x >= 3.5) {
		object2.position.x = 3.45;
		object2.velocity.x *= -1;
	}
	if (object2.position.x <= -3.5) {
		object2.position.x = -3.45;
		object2.velocity.x *= -1;
	}
	if (object2.position.y >= 1.9) {
		object2.position.y = 1.85;
		object2.velocity.y *= -1;
	}
	if (object2.position.y <= -1.9) {
		object2.position.y = -1.85;
		object2.velocity.y *= -1;
	}
	Vector3 p;
	if (checkSATCollision(player.worldVertices, object1.worldVertices, p))
	{
		player.position.x += p.x / 2 * 1.01;
		player.position.y += p.y / 2 * 1.01;

		object1.position.x -= p.x / 2 * 1.01;
		object1.position.y -= p.y / 2 * 1.01;
	}
	if (checkSATCollision(player.worldVertices, object2.worldVertices, p))
	{
		player.position.x += p.x / 2 * 1.01;
		player.position.y += p.y / 2 * 1.01;

		object2.position.x -= p.x / 2 * 1.01;
		object2.position.y -= p.y / 2 * 1.01;
	}
	if (checkSATCollision(object1.worldVertices, object2.worldVertices, p))
	{
		object1.position.x += p.x / 2 * 1.01;
		object1.position.y += p.y / 2 * 1.01;

		object2.position.x -= p.x / 2 * 1.01;
		object2.position.y -= p.y / 2 * 1.01;
	}
	return false;
}

void Update(float elapsed) {
	player.update(elapsed);
	object1.update(elapsed);
	object2.update(elapsed);
	object1.rotation += 1 * elapsed;
	object2.rotation += 1 * elapsed;
	

}

void Render(ShaderProgram& program, float elapsed) {
	glClear(GL_COLOR_BUFFER_BIT);

	Matrix viewMatrix;
	viewMatrix.identity();



	player.Draw(program);
	object1.Draw(program);
	object2.Draw(program);
	program.setViewMatrix(viewMatrix);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	SDL_GL_SwapWindow(displayWindow);
}



enum GameState {Start, Running, GameOver };

int main(int argc, char *argv[])
{
	//Zami Talukder
	//HW4 Intro to Game Programming

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
	Matrix modelMatrix;
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f); //-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0
	
	
	GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	GLuint playerSprite = LoadTexture(RESOURCE_FOLDER"rectangle.png");
	GLuint object1Sprite = LoadTexture(RESOURCE_FOLDER"redRectangle.png");
	player = Entity(playerSprite, .5, 1, Vector3(-2,-1,0));

	object1 = Entity(object1Sprite, .5, 1, Vector3(1, 1, 0));
	object1.velocity = Vector3(1, -2, 0);
	object1.rotation = 30;
	object1.scale = Vector3(2,1,1);

	object2 = Entity(object1Sprite, .5, 1, Vector3(-1, 1, 0));
	object2.velocity = Vector3(3, -2, 0);
	object2.rotation = -60;
	object2.scale = Vector3(2, 1, 1);


	
	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;
	while (!done) {
		
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		done = ProcessEvents(event,elapsed);
		Update(elapsed);
		program.setProjectionMatrix(projectionMatrix);
		Render(program, elapsed);
	}

	SDL_Quit();
	return 0;
}
