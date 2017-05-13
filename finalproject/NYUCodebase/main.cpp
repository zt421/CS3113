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
#include <SDL_mixer.h>

//player states

//multiple levels
//1- just jumping
//2- harder jumping
//3- jumping with enemies

//AI



//title screen (basic)

//tab to pause game or quit (new menu)

//music and sound effects

//animation / particle effects, smoke from feet of ball


using namespace std;

#define TILE_SIZE .25f

class Entity;

SDL_Window* displayWindow;

//stores map data
int mapWidth = -1;
int mapHeight = -1;
unsigned int** levelData;
vector<Entity*> entities;   //STORES ALL ENTITIES

float gunOffset = 4;
float gunInitial = 0;
bool gunForward = true;
float shootingTime = 0;


Mix_Chunk* shooting;


enum GameState { LEVEL1, LEVEL2, LEVEL3, PAUSE, GAMESTART, GAMEOVER };
enum EntityType { ENTITIY_PLAYER, ENTITY_ENEMY, ENTITY_COIN };
enum EntityState { FREE, HURT, JUMPING, FALLING };

GameState beforePause;
GameState gameState;

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






class Entity {
public:
	Entity(GLuint textureID = 0, float size = 1, float aspectRatio = 1, Vector3 position = Vector3(0,0,0),
		TextCoords text = TextCoords(0.0,1.0,0.0,1.0), EntityType type = ENTITIY_PLAYER ): textureID(textureID), size(size), aspectRatio(aspectRatio),
		position(position), textCoords(text), type(type) {
		halfLengths = Vector3(size*aspectRatio*.5, size*.5, 0);
		acceleration = Vector3(0, 0, 0);
		velocity = Vector3(0, 0, 0);
		entityState = FREE;
		rotation = 0;
	}

	Entity(EntityType type, Vector3 position) : type(type), position(position) {
		entityState = FREE;
		bottomTile = 0;
		halfLengths.x = TILE_SIZE;
		halfLengths.y = TILE_SIZE;
	}

	EntityType type;
	EntityState entityState;
	Vector3 position;
	Vector3 acceleration;
	Vector3 velocity;
	Vector3 halfLengths;
	Vector3 friction;
	GLuint textureID;
	TextCoords textCoords;
	vector<Entity*>* entities;

	char bottomTile;

	bool isStatic = false;
	bool visible = true;
	float aspectRatio;  //x/y
	float size;
	float rotation;
	float gravity;

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
		velocity.x += acceleration.x * elapsed;
		velocity.x = lerp(velocity.x, 0, elapsed * friction.x);
		position.x += velocity.x *  elapsed;
		Entity* colliding = checkBoxBoxCollision();
		//moveAway('x', colliding);

		velocity.y += acceleration.y * elapsed;
		velocity.y += gravity * elapsed;
		velocity.y = lerp(velocity.y, 0, elapsed * friction.y);
		position.y += velocity.y *  elapsed;
		colliding = checkBoxBoxCollision();
		//moveAway('y', colliding);
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
					//(*(this->entities))[i]->visible = false;
					if ((*(this->entities))[i]->type == ENTITY_COIN) {

						if (gameState == LEVEL1) gameState = LEVEL2;
						else if (gameState == LEVEL2) gameState = LEVEL3;
						else if (gameState == LEVEL3)  gameState = GAMEOVER;
						(*(this->entities))[i]->visible = false;
						
					}
					
					if (entityState != HURT) {
						velocity.x += 2;
						velocity.y += 2;
						entityState = HURT;
					}
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
		worldToTileCoordinates(position.x, position.y + halfLengths.y, &garb, &top);
		worldToTileCoordinates(position.x, position.y - halfLengths.y, &garb, &bottom);
		worldToTileCoordinates(position.x, position.y , &x, &y);


		float penetration;
		
		if (bottom <= 1 || top <= 1 || top >= mapHeight-1 || bottom >= mapHeight-1)
		{
			velocity.y = 0;
			position.y -= .001;
			return false;
		}

		if (left <= 1 || right <= 1 || left >= mapWidth - 1 || right >= mapWidth - 1)
		{
			velocity.x = 0;
			position.x -= .001;
			return false;
		}
		if (levelData[bottom][x] != 0)
		{
			penetration = fabs(fabs(-TILE_SIZE*bottom) - fabs(position.y - halfLengths.y));
			position.y += penetration + .001;
			velocity.y = 0;
			acceleration.y = 0;
			collidedBottom = true;
			bottomTile = levelData[bottom][x];
		}
		else
			collidedBottom = false;
		

		worldToTileCoordinates(position.x, position.y + halfLengths.y, &garb, &top);
		worldToTileCoordinates(position.x, position.y, &x, &y);
		if (levelData[top][x] != 0 )
		{
			penetration = fabs(fabs(position.y + halfLengths.y) - fabs(((-TILE_SIZE*top) - TILE_SIZE)));
			position.y -= penetration + .001;
			velocity.y = 0;
			acceleration.y = 0;
			collidedTop = true;
		}
		else
			collidedTop = false;


		worldToTileCoordinates(position.x - halfLengths.x, position.y, &left, &garb);
		worldToTileCoordinates(position.x, position.y, &x, &y);
		if (levelData[y][left] != 0 ) //&& collidedBottom)
		{
			penetration = fabs(fabs(position.x - halfLengths.x) - fabs(((TILE_SIZE*left) + TILE_SIZE)));
			position.x += penetration + .001;
			velocity.x = 0;
			acceleration.x = 0;
			collidedLeft = true;
		}
		else
			collidedLeft = false;


		worldToTileCoordinates(position.x + halfLengths.x, position.y, &right, &garb);
		worldToTileCoordinates(position.x, position.y, &x, &y);
		if (levelData[y][right] != 0 ) //&& collidedBottom)
		{
			penetration = fabs(fabs(TILE_SIZE * right) - fabs(position.x + halfLengths.x));
			position.x -= penetration + .001;
			velocity.x = 0;
			acceleration.x = 0;
			collidedRight = true;
		}
		else
			collidedRight = false;
		

		return true;
	}



	
};

GLuint LoadTextureNearestNeighbor(const char *filePath) {
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(image);
	return retTexture;
}

bool readHeader(std::ifstream& stream)
{
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
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
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new unsigned int*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned int[mapWidth];
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
					unsigned int val = (unsigned int)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
					}
					else
						levelData[y][x] = 0;
				}
			}
		}
	}
	return true;
}

Entity player;   //////////////////////player and enemy globals
Entity enemy;
Entity gunImage;
Entity banana;

void placeEntity(const string& type, float x, float y) {
	if (type == "Player")
	{
		player = Entity(ENTITIY_PLAYER, Vector3(x, y, 0));
		player.halfLengths = Vector3(TILE_SIZE/2, TILE_SIZE/2, 0);
		player.friction = Vector3(0, 0, 0);
		player.gravity = -3.7;
		entities.push_back(&player);
		player.entities = &entities;
		
		
	}
	if (type == "Coin")
	{
		enemy = Entity(ENTITY_COIN, Vector3(x, y, 0));
		enemy.halfLengths = Vector3(TILE_SIZE/2, TILE_SIZE/2, 0);
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

bool readFile(string input) {
	ifstream inFile(input);
	string line;
	while (getline(inFile, line)) {
		if (line == "[header]") {
			if (!readHeader(inFile)) {
				return false;
			}
		}
		else if (line == "[layer]") {
			readLayerData(inFile);
		}
		else if (line == "[Object Layer 1]") {
			readEntityData(inFile);
		}

	}
	return true;
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


bool ProcessEvents(SDL_Event& event,float elapsed, int& ballIndex) {
	
	while (SDL_PollEvent(&event)) {
		
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			return true;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_Q)
		{
			player.velocity.y += 1.88;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
			if(gameState == GAMESTART)gameState = LEVEL1;
			if (gameState == PAUSE) gameState = beforePause;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_TAB && (gameState == LEVEL1 || gameState == LEVEL2
			|| gameState == LEVEL3)) {
			beforePause = gameState;
			gameState = PAUSE;
		}
		if (event.key.keysym.scancode == SDL_SCANCODE_Z && gameState == PAUSE) {
			return true;
		}
		

	}

	if (gameState == LEVEL1 || gameState == LEVEL2 || gameState == LEVEL3) {
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		//121,205
		if (keys[SDL_SCANCODE_LEFT] && player.collidedBottom && player.bottomTile == 121)
		{
			player.position.x -= .4 * elapsed;
			ballIndex = (ballIndex - 1) % 4;
		}
		else if (keys[SDL_SCANCODE_LEFT] && player.collidedBottom && player.bottomTile == -51)
		{
			player.position.x -= 2.5 * elapsed;
			ballIndex = (ballIndex - 1) % 4;
		}
		else if (keys[SDL_SCANCODE_LEFT])
		{
			player.position.x -= 1.2 * elapsed;
			ballIndex = (ballIndex - 1) % 4;
		}


		if (keys[SDL_SCANCODE_RIGHT] && player.collidedBottom && player.bottomTile == 121)
		{
			player.position.x += .4 * elapsed;
			ballIndex = (ballIndex + 1) % 4;
		}
		else if (keys[SDL_SCANCODE_RIGHT] && player.collidedBottom && player.bottomTile == -51)
		{
			player.position.x += 2.5 * elapsed;
			ballIndex = (ballIndex + 1) % 4;
		}
		else if (keys[SDL_SCANCODE_RIGHT])
		{
			player.position.x += 1.2 * elapsed;
			ballIndex = (ballIndex + 1) % 4;
		}

		if (keys[SDL_SCANCODE_SPACE] && player.collidedBottom)
		{
			player.velocity.y += 1.88;
			Mix_PlayChannel(-1, shooting, 0);
		}
	}

	

	
	
	return false;
}



bool atRight = true;

void Update(float elapsed) {
	if (gameState == LEVEL1 || gameState == LEVEL2 || gameState == LEVEL3) {
		
		player.update(elapsed);
		player.tileMapCollision();
		if (player.collidedBottom)
		{
			player.entityState = FREE;
			player.velocity.x = 0;
		}
	}
	if (gameState == LEVEL3) {
		shootingTime += elapsed;
		//process hand
		if (shootingTime >= 5) {
			if (gunInitial < 1.25 && gunForward) {
				gunImage.position.x = player.position.x + gunOffset - gunInitial;
				gunInitial += .05;
			}
			else if (gunForward)
			{
				gunForward = false;
				gunInitial = 0;
				gunOffset = 2.85;
				banana.position.x = gunImage.position.x - gunImage.halfLengths.x;
				banana.position.y = gunImage.position.y + .2;

			}

			if (gunInitial < 1.55 && !gunForward)
			{
				gunImage.position.x = player.position.x + gunOffset + gunInitial;
				gunInitial += .05;
			}
			else if (!gunForward)
			{
				gunForward = true;
				gunInitial = 0;
				gunOffset = 4;
				shootingTime = 0;
			}
		}
		gunImage.position.y = player.position.y + 1.6;

		
		float yDif = banana.position.y - player.position.y;
		float xDif = banana.position.x - player.position.x;

		if (xDif >= -1 && atRight) {
			banana.position.x -= 2 * elapsed;
		}
		else
			atRight = false;

		if (xDif <= 1 && !atRight)
			banana.position.x += 2 * elapsed;
		else
			atRight = true;

		//banana.position.y = player.position.y - player.halfLengths.y-banana.halfLengths.y;
		
		if (yDif >= 0)
		{
			banana.position.y -= yDif*elapsed;
		}
		else
			banana.position.y += yDif*elapsed;
			
		

	}


}

void Render(ShaderProgram& program, float elapsed, GLuint spritesheet, GLuint ballSheet, 
	int ballIndex, GLuint openingImage, GLuint endingImage, GLuint pauseImage, GLuint bananaSheet) {
	glClear(GL_COLOR_BUFFER_BIT);

	Matrix viewMatrix;
	viewMatrix.identity();

	

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (gameState == GAMESTART) {
		Matrix modelMatrix;

		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, openingImage);

		float vertices[] = { -2.5, -2.5, 2.5, -2.5, 2.5, 2.5,
			-2.5, -2.5, 2.5, 2.5, -2.5, 2.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] =
		{
			0, 1,
			1, 1,
			1,0,
			0,1,
			1, 0,
			0, 0 };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

	}

	if (gameState == LEVEL1 || gameState == LEVEL2 || gameState == LEVEL3) {
		viewMatrix.Translate(-player.position.x, -player.position.y, 0);
		glBindTexture(GL_TEXTURE_2D, spritesheet);

		drawTiles(&program, 24, 16);
		enemy.drawFromSpriteSheet(program, 136, 24, 16);

		glBindTexture(GL_TEXTURE_2D, ballSheet);
		player.drawFromSpriteSheet(program, ballIndex, 4, 1);
	}
	
	if (gameState == LEVEL3) {
		glBindTexture(GL_TEXTURE_2D, bananaSheet);
		banana.drawFromSpriteSheet(program,0,1,1);

		if (shootingTime >= 5)
			gunImage.Draw(program);
	}

	if (gameState == PAUSE) {
		Matrix modelMatrix;

		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, pauseImage);
		float vertices[] = { -2.5, -2.5, 2.5, -2.5, 2.5, 2.5,
			-2.5, -2.5, 2.5, 2.5, -2.5, 2.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] =
		{
			0, 1,
			1, 1,
			1,0,
			0,1,
			1, 0,
			0, 0 };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	if (gameState == GAMEOVER) {
		Matrix modelMatrix;

		modelMatrix.identity();
		program.setModelMatrix(modelMatrix);

		glBindTexture(GL_TEXTURE_2D, endingImage);
		float vertices[] = { -2.5, -2.5, 2.5, -2.5, 2.5, 2.5,
			-2.5, -2.5, 2.5, 2.5, -2.5, 2.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] =
		{
			0, 1,
			1, 1,
			1,0,
			0,1,
			1, 0,
			0, 0 };

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	program.setViewMatrix(viewMatrix);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);

	SDL_GL_SwapWindow(displayWindow);
}



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
	FILE *stream;
	AllocConsole();
	freopen_s(&stream,"CONIN$", "r", stdin);
	freopen_s(&stream,"CONOUT$", "w", stdout);
	freopen_s(&stream,"CONOUT$", "w", stderr);


	glViewport(0, 0, 640, 360);
	ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	
	Matrix modelMatrix;
	projectionMatrix.setOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f); //-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0
	
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Music* music = Mix_LoadMUS("bgm.mp3");
	shooting = Mix_LoadWAV("shoot.wav");
	Mix_PlayMusic(music, -1);
	
	GLuint spritesheet = LoadTextureNearestNeighbor(RESOURCE_FOLDER"dirt-tiles.png");
	GLuint ballsheet = LoadTextureNearestNeighbor(RESOURCE_FOLDER"ball.png");
	GLuint gunSheet = LoadTextureNearestNeighbor(RESOURCE_FOLDER"gunImage.png");
	GLuint bananaSheet = LoadTextureNearestNeighbor(RESOURCE_FOLDER"banana.png");
	GLuint openingImage = LoadTextureNearestNeighbor(RESOURCE_FOLDER"opening.png");
	GLuint endingImage = LoadTextureNearestNeighbor(RESOURCE_FOLDER"ending.png");
	GLuint pauseImage = LoadTextureNearestNeighbor(RESOURCE_FOLDER"pause.png");

	gunImage = Entity(gunSheet, 1.0, 1.4823);


	string levelFile = "level1.txt";
	if (!readFile(levelFile))
		return 0;
	banana = Entity(ENTITY_ENEMY, Vector3(0, 0, 0));
	banana.halfLengths = Vector3(TILE_SIZE / 2, TILE_SIZE / 2, 0);
	entities.push_back(&banana);

	gameState = GAMESTART;
	glUseProgram(program.programID);
	int ballIndex = 0;
	bool loaded2 = false;
	bool loaded3 = false;
	float lastFrameTicks = 0.0f;
	while (!done) {
		
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		

		done = ProcessEvents(event,elapsed,ballIndex);
		Update(elapsed);
		if (gameState == LEVEL2 && !loaded2) {
			for (int i = 0; i < mapHeight; i++)
			{
				delete levelData[i];
			}
			delete levelData;
			readFile("level2.txt");
			loaded2 = true;
		}
		if (gameState == LEVEL3 && !loaded3) {
			for (int i = 0; i < mapHeight; i++)
			{
				delete levelData[i];
			}
			delete levelData;
			readFile("level3.txt");
			loaded3 = true;
		}
		program.setProjectionMatrix(projectionMatrix);
		Render(program, elapsed, spritesheet,ballsheet, ballIndex, openingImage, endingImage,
			pauseImage, bananaSheet);
	}
	for (int i = 0; i < mapHeight; i++)
	{
		delete levelData[i];
	}
	delete levelData;

	SDL_Quit();
	return 0;
}
