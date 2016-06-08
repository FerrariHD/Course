#define _CRT_SECURE_NO_WARNINGS

#include <GL\glew.h>
#include <GL\glut.h>
#include <ctime>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

int player0[2] = { 64, 32 };
int player1[2] = { 224, 32 };
int player2[2] = { 0, 32 };
int player3[2] = { 160, 32 };

int enemy0[2] = { 64, 64 };
int enemy1[2] = { 224, 64 };
int enemy2[2] = { 0, 64 };
int enemy3[2] = { 127, 64 };

int shell0[2] = { 660, 202 };
int shell1[2] = { 693, 202 };
int shell2[2] = { 643, 202 };
int shell3[2] = { 678, 202 };

// размеры карты
const int H = 25;
const int W = 34;

// таймер для стрельбы первого и второго игроков
float shootTimerFirst = 0;
bool firstIsKilled = false;
int firstScore = 0;

class Shell;
class Map;

struct Image {
	int sizeX;
	int sizeY;
	unsigned char *data;
};


class Painter
{
private:
	/* storage for one texture  */
	GLuint texture[1];
	char texPath[80];
	int bitmapHeader;
	unsigned int bitmapVersion;
	int height;
	unsigned int compression;
	unsigned int pixelDataOffset;
	unsigned int sizeImage;
	unsigned short int bpp;

public:
	Painter()
	{
		strcpy(texPath, "NES_-_Battle_City_-_General_Sprites.bmp");
		bitmapHeader = 14;
	}

	int imageLoad(char *filename, Image *image) {
		FILE *file;
		unsigned int i;                    
		unsigned short int planes;          // количество слоёв(должно быть 1)


		// проверяем загрузился ли файл
		if ((file = fopen(filename, "rb")) == NULL)
		{
			printf("File Not Found : %s\n", filename);
			return 1;
		}

		
		fseek(file, bitmapHeader - sizeof(pixelDataOffset), SEEK_CUR);

		if ((i = fread(&pixelDataOffset, 4, 1, file)) != 1) {
			printf("Error reading pixelDataOffset from %s.\n", filename);
			return 1;
		}

		if ((i = fread(&bitmapVersion, 4, 1, file)) != 1) {
			printf("Error reading width from %s.\n", filename);
			return 1;
		}

		// считываем ширину
		if ((i = fread(&image->sizeX, 4, 1, file)) != 1) {
			printf("Error reading width from %s.\n", filename);
			return 1;
		}

		// считываем высоту
		if ((i = fread(&height, 4, 1, file)) != 1) {
			printf("Error reading height from %s.\n", filename);
			return 1;
		}
		image->sizeY = abs(height);

		// считываем кол-во слоёв
		if ((fread(&planes, 2, 1, file)) != 1) {
			printf("Error reading planes from %s.\n", filename);
			return 1;
		}

		// считываем кол-во битов
		if ((i = fread(&bpp, 2, 1, file)) != 1) {
			printf("Error reading bpp from %s.\n", filename);
			return 1;
		}

		// считываем сжатие
		if ((i = fread(&compression, 4, 1, file)) != 1) {
			printf("Error reading compression from %s.\n", filename);
			return 1;
		}

		// считываем размер изображения
		if ((i = fread(&sizeImage, 4, 1, file)) != 1) {
			printf("Error reading sizeImage from %s.\n", filename);
			return 1;
		}

		
		fseek(file, pixelDataOffset, SEEK_SET);

		// выделяем место для считывания данных
		image->data = (unsigned char *)malloc(sizeof(unsigned char)*sizeImage);
		if (image->data == NULL) {
			printf("Error allocating memory for color-corrected image data\n");
			return 1;
		}

		// считываем данные
		if ((i = fread(image->data, sizeImage, 1, file)) != 1) {
			printf("Error reading image data from %s.\n", filename);
			return 1;
		}

		// переворачиваем цвет из bgr в rgb
		unsigned char temp;
		for (i = 0; i < sizeImage; i += bpp / 8) {
			temp = image->data[i];
			image->data[i] = image->data[i + 2];
			image->data[i + 2] = temp;
		}

		// переворачиваем оси
		for (int i = 0; i < image->sizeY / 2; i++) {
			for (int j = 0; j < image->sizeX; j++) {
				for (int z = 0; z < bpp / 8; z++) {
					temp = image->data[i * image->sizeX * bpp / 8 + j*bpp / 8 + z];
					image->data[i * image->sizeX * bpp / 8 + j*bpp / 8 + z] = image->data[(image->sizeY - 2 - i) * image->sizeX * bpp / 8 + j*bpp / 8 + z];
					image->data[(image->sizeY - 2 - i) * image->sizeX * bpp / 8 + j*bpp / 8 + z] = temp;
				}
			}
		}

		// we're done
		return 0;
	}

	void loadGLTextures() {
		Image *image1;

		// выделяем место под текстуру
		image1 = (Image *)malloc(sizeof(Image));
		if (image1 == NULL) {
			printf("Error allocating space for image");
		}

		if (imageLoad(texPath, image1) != 0) {
			exit(1);
		}

		glEnable(GL_TEXTURE_RECTANGLE_NV);
		// создаём текстуру  
		glGenTextures(1, &texture[0]);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, texture[0]);   // 2д текстура

		glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // scale linearly when image bigger than texture
		glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // scale linearly when image smalled than texture

		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, bpp / 8, image1->sizeX, image1->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, image1->data);
		glDisable(GL_TEXTURE_RECTANGLE_NV);
	}

	// главная функция отрисовки
	void square(int x1, int y1, int x2, int y2, int* startXY, int size)
	{
		glEnable(GL_TEXTURE_RECTANGLE_NV);
		glEnable(GL_ALPHA);
		glEnable(GL_BLEND);
		//белый "фон"
		glColor3f(1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, texture[0]);   // выбираем текстуру, которую будем использовать

		
		glBegin(GL_QUADS);                      // выбираем отрисовку квадрата

		//верхний левый
		glTexCoord2i(startXY[0], startXY[1]);
		glVertex2f(x1, y1);  
		//верхний правый
		glTexCoord2i(startXY[0] + size, startXY[1]);
		glVertex2f(x2, y1);  
		// нижний правый
		glTexCoord2i(startXY[0] + size, startXY[1] + size);
		glVertex2f(x2, y2);  
		// нижний левый
		glTexCoord2i(startXY[0], startXY[1] + size);
		glVertex2f(x1, y2);  


		glEnd();                                    // завершаем отрисовку
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA);
		glDisable(GL_TEXTURE_RECTANGLE_NV);
	}
};

class Map			// класс отрисовки карты
{
public:
	enum Type { EMPTY, BRICK, GRASS, WALL, WATER };
	Type m_[H][W];
	int brick[2] = { 512, 0 };
	int grass[2] = { 545, 64 };
	int water[2] = { 512, 64 };
	int wall[2] = { 512, 32 };
	int size = 30;
	int randBlock;

	Map()
	{
		srand(time(NULL));
		ifstream fin;
		fin.open("Map.txt");
		string str;
		char c;
		for (int y = 0; y < H; ++y) {
			getline(fin, str);
			for (int x = 0; x < W; ++x) {
				c = str[x];
				if (c == 'B') {
					m_[y][x] = WALL;
					continue;
				}
				if (c == 'R') {
					randBlock = rand() % 15;
					switch (randBlock)
					{
					case 1: m_[y][x] = WATER; continue;
					case 2: m_[y][x] = WALL; continue;
					case 3: m_[y][x] = BRICK; continue;
					case 4: m_[y][x] = GRASS; continue;
					case 5: m_[y][x] = BRICK; continue;
					case 6: m_[y][x] = GRASS; continue;
					case 7: m_[y][x] = WATER; continue;
					}
				}

				if (c == ' ') {
					m_[y][x] = EMPTY;
					continue;
				}
			}
		}
		fin.close();
	}

	// проходим по матрице нашей карты, выбираем соответствующий тип, смещаем позицию, отрисовываем карту
	void renderMap(Painter &p)
	{
		for (int y = 0; y < H; ++y) {
			for (int x = 0; x < W; ++x) {
				switch (m_[y][x]) {

				case EMPTY:
					break;

				case BRICK:
					p.square(x * 30, y * 30,
						(x + 1) * 30, (y + 1) * 30,
						brick, size);
					break;

				case WALL:
					p.square(x * 30, y * 30,
						(x + 1) * 30, (y + 1) * 30,
						wall, size);
					break;

				case WATER:
					p.square(x * 30, y * 30,
						(x + 1) * 30, (y + 1) * 30,
						water, size);
					break;

				case GRASS:
					p.square(x * 30, y * 30,
						(x + 1) * 30, (y + 1) * 30,
						grass, size);
					break;
				}
			}
		}
	}

	Type getBlock(int x, int y)
	{
		return m_[x][y];
	}

	void setBlock(int x, int y)
	{
		m_[x][y] = EMPTY;
	}

	~Map()
	{

	}

};

class Entity				// обобщённый класс "Объектов"
{
protected:
	int currentAxis;		// текущее направление движения
	int axis;				// прошлое/текущее направление движения. используется для правильного выстрела в состоянии остановки
	string name;			// имя у объекта
	int health;
	bool life;
	float dx, dy;			// для передвижения по Х и У


public:

	string getName()
	{
		return name;
	}

	void setHealth(int damage)
	{
		health -= damage;
	}

	void setLife()
	{
		life = false;
	}

	int getLife()
	{
		return life;
	}

	void setDx(float DX)
	{
		dx = DX;
	}

	void setDy(float DY)
	{
		dy = DY;
	}

	int getAxis()
	{
		return axis;
	}

	~Entity()
	{

	}

};

Map map;

class Player : public Entity				// класс главных игроков
{
private:
	int score;							// кол-во набранных очков
	float coordY = 690;
	float coordX = 400;
	int cooldown;			// время перезарядки
public:

	Player()
	{

	}

	Player(string Name)
	{
		cooldown = 150;				// указываем время перезарядки для главных игроков
		life = true;
		axis = 3;						// по началу смотрим вверх
		score = 0;
		health = 100;
		name = Name;
		dx = dy = 0;
		currentAxis = 0;

		float coordY = 690;
		float coordX = 400;
	}

	//главный метод обновления игроков
	void update()
	{
		coordX += dx;							// смещаем игрока по Х
		Collision(0);			// проверяем столкновения с объектами по Х
		coordY += dy;							// смещаем по У
		Collision(1);			// проверяем столкновения с объектами по У

		shootTimerFirst++;

		if (health <= 0)
			life = false;

		if (cooldown < 100)								// минимальное время перезарядки = 100
			cooldown = 100;


		//если танк не стоит на месте, то запоминаем его направление для стрельбы
		if (currentAxis != 0)
		{
			axis = currentAxis;
		}

		currentAxis = 0;
		//dx = 0;
		//dy = 0;
	}

	// метод проверки игрока на столкновения с объектами карты. в случае наезда просто выталкиваем его обратно,
	// либо подбираем бонус(если объект на который вы наехали является бонусом)
	void Collision(int axis)
	{
		for (int i = coordY / 30; i < (coordY + 25) / 30; i++)
			for (int j = coordX / 30; j < (coordX + 25) / 30; j++)
			{
				if ((map.getBlock(i, j) == map.BRICK) || (map.getBlock(i, j) == map.WALL))
				{
					if (axis == 0)
					{
						if (dx > 0) coordX = j * 30 - 25;
						if (dx < 0) coordX = j * 30 + 30;
					}
					if (axis == 1)
					{
						if (dy > 0) coordY = i * 30 - 25;
						if (dy < 0) coordY = i * 30 + 30;
					}
				}
			}
	}

	void setCurrentAxis(int axis)
	{
		currentAxis = axis;
	}

	int getCurrentAxis()
	{
		return currentAxis;
	}

	int getShellAxis()
	{
		return axis;
	}

	float getRectLeft()
	{
		return coordX;
	}

	float getRectTop()
	{
		return coordY;
	}

	void setScore(int Score)
	{
		score += Score;
	}

	int getScore()
	{
		return score;
	}

	int getHealth()
	{
		return health;
	}

	int getCooldown()
	{
		return cooldown;
	}

	~Player()
	{

	}

};

class Shell : public Entity				// класс "Снаряд"
{
private:
	int direction;						// направление движения снаряда
	string shotowner;					// имя "владельца" снаряда. то есть тот, кто выстрелил
	float coordY;
	float coordX;
public:

	Shell()
	{

	}

	Shell(int dir, float x, float y, string shotOwner)
	{
		coordX = x;
		coordY = y;
		life = true;
		direction = dir;					// направление движения снаряда
		shotowner = shotOwner;
		name = "Shell";

		// в зависимости от направления движения указываем его скорость, начальные координаты и изменяем картинку
		switch (direction)
		{
		case 1: dx = -3; dy = 0; coordX -= 7; coordY += 7.5; break;
		case 2: dx = 3; dy = 0; coordX += 30; coordY += 8; break;
		case 3: dx = 0; dy = -3; coordX += 9; coordY -= 5; break;
		case 4: dx = 0; dy = 3; coordX += 9; coordY += 29; break;
		}

	}

	// главный метод обновления снаряда
	void update()
	{
		coordX += dx;			// изменяем положение по Х
		coordY += dy;			// ихменяем положение по У
		Collision();		// проверяем на столкновения со стенами
	}

	// проверка на столкновения с объектами окружения.
	// проверка осуществляется по двум точкам снаряда. отсчёт от левого верхнего угла
	// в случае если попали и стрелял игрок, то воспроизводим подходящий звук в зависимости от типа стены
	void Collision()
	{
		for (int i = coordY / 30; i < (coordY + 5) / 30; i++)
			for (int j = coordX / 30; j < (coordX + 5) / 30; j++)
			{
				if ((map.getBlock(i, j) == map.BRICK) || (map.getBlock(i, j) == map.WALL))
				{
					life = false;
					if (map.getBlock(i, j) == map.BRICK)
					{
						map.setBlock(i, j);
					}
				}
			}
	}

	string getShotOwner()
	{
		return shotowner;
	}

	float getRectLeft()
	{
		return coordX;
	}

	float getRectTop()
	{
		return coordY;
	}

	~Shell()
	{

	}
};

// список для хранения снарядов, а также итератор для работы с ним
list<Shell*> shells;
list<Shell*>::iterator itShell;

class Enemy : public Entity				// класс врага
{
private:
	float shootTimer;					// таймер для стрельбы
	float moveTimer;					// минимальное время для вызова рандомного изменения направления движения
	int randAxis;
	float coordY = 300;
	float coordX = 100;
	int cooldown;			// время перезарядки
public:

	Enemy()
	{

	}

	Enemy(string Test)
	{
		moveTimer = 0;
		shootTimer = 0;
		axis = 2;							// начальное направление для стрельбы
		dx = 1;							// начальное движение
		dy = 0;
		life = true;
		float coordY = 100;
		float coordX = 100;
		health = 100;
		cooldown = 200;
	}

	// главный метод обновления врагов
	void update()
	{
		// увеличиваем время с последней рандомной смены направления.
		// в случае превышения времени в 20000, обнуляем таймер и генерируем новое направление(может совпасть со старым)
		moveTimer++;
		if (moveTimer > 180)
		{
			moveTimer = 0;
			randAxis = rand() % 4;
			if (randAxis == 0)
			{
				dx = 1; dy = 0;
				axis = 2;
			}
			if (randAxis == 1)
			{
				dx = -1; dy = 0;
				axis = 1;
			}
			if (randAxis == 2)
			{
				dy = 1; dx = 0;
				axis = 4;
			}
			if (randAxis == 3)
			{
				dy = -1; dx = 0;
				axis = 3;
			}
		}

		coordX += dx;			// изменяем координаты по Х
		Collision(0);						// проверяем на столкновения с объектами по Х
		coordY += dy;			// изменяем координаты по У
		Collision(1);						// проверяем на столкновения с объектами по У

		if (health <= 0)
			life = false;

		shootTimer++;
		if (shootTimer >= cooldown)
		{
			shootTimer = 0;
			shells.push_back(new Shell(axis, coordX, coordY, " "));
		}
	}

	// проверка на столкновение с объектами игрового окружения.
	// в случае столкновения выталкиваем игрока и увеличиваем шансы на вызов рандомной смены направление движения
	void Collision(int axis)
	{
		for (int i = coordY / 30; i < (coordY + 25) / 30; i++)
			for (int j = coordX / 30; j < (coordX + 25) / 30; j++)
			{
				if ((map.getBlock(i, j) == map.BRICK) || (map.getBlock(i, j) == map.WALL))
				{
					moveTimer++;
					if (axis == 0)
					{
						if (dx > 0) coordX = j * 30 - 25;
						if (dx < 0) coordX = j * 30 + 30;
					}
					if (axis == 1)
					{
						if (dy > 0) coordY = i * 30 - 25;
						if (dy < 0) coordY = i * 30 + 30;
					}
				}
			}
	}

	float getRectLeft()
	{
		return coordX;
	}

	float getRectTop()
	{
		return coordY;
	}

	int getHealth()
	{
		return health;
	}

	~Enemy()
	{

	}

};

// список для хранения врагов, а также итераторы для работы с ним
list<Enemy*> enemies;
list<Enemy*>::iterator itEnemy;

// список для хранения игроков, а также итераторы для работы с ним
list<Player*> players;
list<Player*>::iterator itPlayer;

class Spawner			// отвечает за рандомизацию спауна врагов
{
private:
	int access;			// доступ к спауну
	float spawnTimer;		// таймер

public:

	Spawner()
	{
		access = 1;
		spawnTimer = 0;
	}

	void addNewEnemy()
	{
		access = rand() % 3;
		switch (access)
		{
		case 0: access = 1; break;
		case 1: access = 1; break;
		case 2: access = 0; break;
		}
	}

	void setSpawnTimer()
	{
		// если прошло достаточно времени с момента прошлого вызова спауна
		spawnTimer++;
		if (spawnTimer > 300)
		{
			spawnTimer = 0;
			addNewEnemy();
		}
	}

	int getAccess()
	{
		return access;
	}

	void resetAccess()
	{
		access = 0;
	}

	~Spawner()
	{

	}
};

class DetectHIT		// класс проверки попадания снаряда в игроков/врагов
{
public:

	DetectHIT()
	{

	}

	bool intersects(float x1, float y1, float x2, float y2)
	{
		return
			x1 < x2 + 25 &&
			x1 + 5 > x2 &&
			y1 < y2 + 25 &&
			y1 + 5 > y2;
	}

	void detectHit()
	{
		// проходим по списку снарядов
		for (itShell = shells.begin(); itShell != shells.end(); itShell++)
		{
			// если стрелял игрок
			if ((*itShell)->getShotOwner() != " ")
			{
				// проходим по списку врагов
				for (itEnemy = enemies.begin(); itEnemy != enemies.end(); itEnemy++)
				{
					if (intersects((*itShell)->getRectLeft(), (*itShell)->getRectTop(), (*itEnemy)->getRectLeft(), (*itEnemy)->getRectTop()))
					{
						// отнимаем здоровье и уничтожаем снаряд
						(*itEnemy)->setHealth(50);
						(*itShell)->setLife();

						if ((*itEnemy)->getHealth() <= 0)
						{
							for (itPlayer = players.begin(); itPlayer != players.end(); itPlayer++)
							{
								(*itPlayer)->setScore(1);
							}
						}

						break;
					}
				}
			}

			if ((*itShell)->getShotOwner() == " ")
			{
				// проходим по игрокам
				for (itPlayer = players.begin(); itPlayer != players.end(); itPlayer++)
				{
					if (intersects((*itShell)->getRectLeft(), (*itShell)->getRectTop(), (*itPlayer)->getRectLeft(), (*itPlayer)->getRectTop()))
					{
						(*itPlayer)->setHealth(30);
						(*itShell)->setLife();
						break;
					}
				}
			}
		}
	}

	~DetectHIT()
	{

	}

};

Spawner spawn;
DetectHIT detecthit;
Painter p;

void Keyboard(unsigned char key, int x, int y)
{
	itPlayer = players.begin();
	switch (key)
	{
	case 32:
		if (shootTimerFirst >= (*itPlayer)->getCooldown())
		{
			shootTimerFirst = 0;
			shells.push_back(new Shell((*itPlayer)->getAxis(), (*itPlayer)->getRectLeft(), (*itPlayer)->getRectTop(), "firstplayer"));
			break;
		}
	}
}

void Keyboard2(int key, int x, int y)
{
	itPlayer = players.begin();
	switch (key)
	{
	case GLUT_KEY_UP:
	{
		(*itPlayer)->setDx(0);
		(*itPlayer)->setDy(-1);
		(*itPlayer)->setCurrentAxis(3);
		break;
	}
	case GLUT_KEY_DOWN:
	{
		(*itPlayer)->setDx(0);
		(*itPlayer)->setDy(1);
		(*itPlayer)->setCurrentAxis(4);
		break;
	}
	case GLUT_KEY_LEFT:
	{
		(*itPlayer)->setDx(-1);
		(*itPlayer)->setDy(0);
		(*itPlayer)->setCurrentAxis(1);
		break;
	}
	case GLUT_KEY_RIGHT:
	{
		(*itPlayer)->setDx(1);
		(*itPlayer)->setDy(0);
		(*itPlayer)->setCurrentAxis(2);
		break;
	}
	}
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);
	map.renderMap(p);
	for (itPlayer = players.begin(); itPlayer != players.end(); itPlayer++)
	{
		switch ((*itPlayer)->getAxis())
		{
		case 1: p.square((*itPlayer)->getRectLeft(), (*itPlayer)->getRectTop(), ((*itPlayer)->getRectLeft() + 25), ((*itPlayer)->getRectTop() + 25), player0, 30); break;
		case 2: p.square((*itPlayer)->getRectLeft(), (*itPlayer)->getRectTop(), ((*itPlayer)->getRectLeft() + 25), ((*itPlayer)->getRectTop() + 25), player1, 30); break;
		case 3: p.square((*itPlayer)->getRectLeft(), (*itPlayer)->getRectTop(), ((*itPlayer)->getRectLeft() + 25), ((*itPlayer)->getRectTop() + 25), player2, 30); break;
		case 4: p.square((*itPlayer)->getRectLeft(), (*itPlayer)->getRectTop(), ((*itPlayer)->getRectLeft() + 25), ((*itPlayer)->getRectTop() + 25), player3, 30); break;
		}
	}
	for (itEnemy = enemies.begin(); itEnemy != enemies.end(); itEnemy++)
	{
		switch ((*itEnemy)->getAxis())
		{
		case 1: p.square((*itEnemy)->getRectLeft(), (*itEnemy)->getRectTop(), ((*itEnemy)->getRectLeft() + 25), ((*itEnemy)->getRectTop() + 25), enemy0, 30); break;
		case 2: p.square((*itEnemy)->getRectLeft(), (*itEnemy)->getRectTop(), ((*itEnemy)->getRectLeft() + 25), ((*itEnemy)->getRectTop() + 25), enemy1, 30); break;
		case 3: p.square((*itEnemy)->getRectLeft(), (*itEnemy)->getRectTop(), ((*itEnemy)->getRectLeft() + 25), ((*itEnemy)->getRectTop() + 25), enemy2, 30); break;
		case 4: p.square((*itEnemy)->getRectLeft(), (*itEnemy)->getRectTop(), ((*itEnemy)->getRectLeft() + 25), ((*itEnemy)->getRectTop() + 25), enemy3, 30); break;
		}
	}
	for (itShell = shells.begin(); itShell != shells.end(); itShell++)
	{
		 p.square((*itShell)->getRectLeft(), (*itShell)->getRectTop(), ((*itShell)->getRectLeft() + 5), ((*itShell)->getRectTop() + 5), shell0, 5);
	}
	glutSwapBuffers();
}

void timerUpdateLogic(int = 0)
{
	spawn.setSpawnTimer();
	if (spawn.getAccess() == 1)
	{
		spawn.resetAccess();
		enemies.push_back(new Enemy("Test"));
	}

	// вызов главного метода обновления
	// в случае уничтожения игрока проверяем имя, заглушаем звук движения, изменяем флаг и записываем кол-во набранных очков
	// также удаляем игрока со списка
	for (itPlayer = players.begin(); itPlayer != players.end();)
	{
		(*itPlayer)->update();

		if ((*itPlayer)->getLife() == false)
		{
			firstIsKilled = true;
			firstScore = (*itPlayer)->getScore();
			itPlayer = players.erase(itPlayer);
		}
		else
			itPlayer++;
	}

	// вызов главного метода обновления врагов.
	// в случае уничтожения удаляем его из списка
	for (itEnemy = enemies.begin(); itEnemy != enemies.end();)
	{
		(*itEnemy)->update();

		if ((*itEnemy)->getLife() == false)
			itEnemy = enemies.erase(itEnemy);
		else
			itEnemy++;
	}

	// вызов главного метода обновления снарядов.
	// в случае уничтожения удаляем его из списка
	for (itShell = shells.begin(); itShell != shells.end();)
	{
		(*itShell)->update();

		if ((*itShell)->getLife() == false)
			itShell = shells.erase(itShell);
		else
			itShell++;
	}

	// проверяем попадание снаряда в игроков/врагов
	detecthit.detectHit();

	glutTimerFunc(10, timerUpdateLogic, 0);
}

void timerRender(int = 0)
{
	display();
	glutTimerFunc(16, timerRender, 0);
}

int main(int argc, char **argv)
{
	srand(time(NULL));		// для правильной работы рандома

	players.push_back(new Player("firstplayer"));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(1020, 750);
	glutInitWindowPosition(150, 100);
	glutCreateWindow("BattleCity Survival");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0, 0, 0, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1020, 750, 0, -1.0, 1.0);
	glutDisplayFunc(display);
	p.loadGLTextures();
	timerUpdateLogic();
	timerRender();
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(Keyboard2);
	glutMainLoop();
	return 0;
}