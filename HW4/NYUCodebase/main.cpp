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

using namespace std;




#define TILE_SIZE .25f

class Entity;

SDL_Window* displayWindow;

//stores map data
int mapWidth = 5;
int mapHeight = 5; 
unsigned char** levelData;  
vector<Entity*> entities;   //STORES ALL ENTITIES




struct Vector3 {
	Vector3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
	float x;
	float y;
	float z;
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

void worldToTileCoordinates(float worldX, float worldY, int* gridX, int* gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
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
	Vector3 acceleration;
	Vector3 velocity;
	Vector3 halfLengths;
	Vector3 friction;
	GLuint textureID;
	TextCoords textCoords;
	vector<Entity*>* entities;

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
	void drawFromSpriteSheet(ShaderProgram& program, int spriteIndex, int SPRITE_COUNT_X, int SPRITE_COUNT_Y )
	{
		if (!visible) return;
		float u = (float)(((int)spriteIndex) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
		float v = (float)(((int)spriteIndex) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

		float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
		float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

		float vertices[] = {

			TILE_SIZE , -TILE_SIZE ,
			TILE_SIZE , (-TILE_SIZE ) - TILE_SIZE,
			(TILE_SIZE ) + TILE_SIZE, (-TILE_SIZE ) - TILE_SIZE,

			TILE_SIZE , -TILE_SIZE ,
			(TILE_SIZE ) + TILE_SIZE, (-TILE_SIZE ) - TILE_SIZE,
			(TILE_SIZE ) + TILE_SIZE, -TILE_SIZE 
		};

		float texCoords[] = {
			u, v,
			u, v + (spriteHeight),
			u + spriteWidth, v + (spriteHeight),

			u, v,
			u + spriteWidth, v + (spriteHeight),
			u + spriteWidth, v
		};

		Matrix modelMatrix;
		modelMatrix.identity();
		modelMatrix.Translate(position.x, position.y, position.z);

		program.setModelMatrix(modelMatrix);


		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	
	void update(float elapsed)
	{
		tileMapCollision();
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

	//used to check collision between entity and tile map
	bool tileMapCollision()
	{
		int top, bottom,left,right,x,y,garb;
		worldToTileCoordinates(position.x + halfLengths.x, position.y, &right, &garb);
		worldToTileCoordinates(position.x - halfLengths.x, position.y, &left, &garb);
		worldToTileCoordinates(position.x , position.y + halfLengths.y, &garb, &top);
		worldToTileCoordinates(position.x, position.y - halfLengths.y, &garb, &bottom);
		worldToTileCoordinates(position.x, position.y , &x, &y);


		float penetration;
		
		
		
		
		if (bottom <= 1 || top <= 1 || top >= mapHeight-1 || bottom >= mapHeight-1)
		{
			velocity.y = 0;
			position.y -= .001;
		}

		if (left <= 1 || right <= 1 || left >= mapWidth - 1 || right >= mapWidth - 1)
		{
			velocity.x = 0;
			position.x -= .001;
		}

		if (levelData[bottom][x] != 0)
		{
			penetration = fabs(fabs(-TILE_SIZE*bottom) - fabs(position.y - halfLengths.y));
			position.y += penetration + .001;
			velocity.y = 0;
			collidedBottom = true;
		}
		
		if (levelData[top][x] != 0)
		{
			penetration = fabs(fabs(position.y + halfLengths.y) - fabs(((-TILE_SIZE*top)-TILE_SIZE)));
			position.y -= penetration + .001;
			velocity.y = 0;
			collidedTop = true;
		}
		if (levelData[y][left] != 0)
		{
			penetration = fabs(fabs(position.x - halfLengths.x) - fabs(((TILE_SIZE*left) + TILE_SIZE)));
			position.x += penetration + .001;
			velocity.x = 0;
			collidedLeft = true;
		}
		if (levelData[y][right] != 0)
		{
			penetration = fabs(fabs(TILE_SIZE * right) - fabs(position.x+halfLengths.x));
			position.x -= penetration + .001;
			velocity.x = 0;
			collidedRight = true;
		}
		
		return true;
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





bool readHeader(std::ifstream& stream)
{
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while(getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if(mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned char*[mapHeight];
		for	(int i =0; i < mapHeight; ++i) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true;
	}
}

bool readLayerData(std::ifstream& stream)
{
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					unsigned char val = (unsigned char)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else {
						levelData[y][x] = 0;
					}
				}
			}
		}
	}
	return true;
}

Entity player;   //////////////////////player and enemy globals
Entity enemy;

void placeEntity(const string& type, float x, float y) {
	if (type == "Player")
	{
		player = Entity(ENTITIY_PLAYER, Vector3(x, y, 0));
		player.halfLengths = Vector3(TILE_SIZE / 2, TILE_SIZE / 2, 0);
		player.acceleration = Vector3(0, -1, 0);
		player.friction = Vector3(1, 0, 0);
		entities.push_back(&player);
		player.entities = &entities;
		
		
	}
	if (type == "Enemy")
	{
		enemy = Entity(ENTITY_ENEMY, Vector3(x, y, 0));
		enemy.halfLengths = Vector3(TILE_SIZE / 2, TILE_SIZE / 2, 0);
		entities.push_back(&enemy);
		enemy.entities = &entities;
	}
}

bool readEntityData(std::ifstream& stream)
{
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');
			float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

void readFile() {
	ifstream inFile("mymap.txt");
	string line;
	while (getline(inFile, line)) {
		if (line == "[header]") {
			if (!readHeader(inFile)) {
				return;
			}
		}
		else if (line == "[layer]") {
			readLayerData(inFile);
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(inFile);
		}

	}
}


void drawTiles(ShaderProgram* program, const int SPRITE_COUNT_X, const int SPRITE_COUNT_Y)
{
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {
			if (levelData[y][x] != 0) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;

				float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
				float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;

				vertexData.insert(vertexData.end(), {

					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,

					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});

				texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),

					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
				});
			}
		}
	}
	
	Matrix modelMatrix;
	modelMatrix.identity();
	modelMatrix.Translate(TILE_SIZE*1.5, -TILE_SIZE*1.5, 0);
	program->setModelMatrix(modelMatrix);

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);

	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);

}

bool ProcessEvents(SDL_Event& event,float elapsed) {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			return true;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_UP)
		{

			//player.position.y = player.position.y + player.halfLengths.y;
			player.velocity.y += 3 * elapsed;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)
		{
			player.velocity.y -= 3.0 * elapsed;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)
		{
			player.velocity.x -= 3.0 * elapsed;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)
		{
			player.velocity.x += 3.0 * elapsed;
		}
	}
	return false;
}

void Update(float elapsed) {
	player.update(elapsed);
}

void Render(ShaderProgram& program, float elapsed, GLuint spritesheet) {
	glClear(GL_COLOR_BUFFER_BIT);

	Matrix viewMatrix;
	viewMatrix.identity();

	viewMatrix.Translate(-player.position.x, -player.position.y, 0);

	

	glBindTexture(GL_TEXTURE_2D, spritesheet);

	drawTiles(&program, 16, 8);
	player.drawFromSpriteSheet(program, 80, 16, 8);
	enemy.drawFromSpriteSheet(program, 59, 16, 8);



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
	
	
	GLuint spritesheet = LoadTexture(RESOURCE_FOLDER"arne_sprites.png"); 
	GLuint fontSheet = LoadTexture(RESOURCE_FOLDER"font1.png");
	
	readFile();
	
	glUseProgram(program.programID);

	float lastFrameTicks = 0.0f;
	while (!done) {
		
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		done = ProcessEvents(event,elapsed);
		Update(elapsed);
		program.setProjectionMatrix(projectionMatrix);
		Render(program, elapsed, spritesheet);
		DrawText1(&program, fontSheet, "Please work", .3, 0, -3,0);
	}

	SDL_Quit();
	return 0;
}
