#ifndef ATTACK_CONTROLLER
#define ATTACK_CONTROLLER

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES

#include "AdditionalInfo.cpp"

#define NONE 0
#define MY_CAR 1
#define TEAMMATE_CAR 2
#define ENEMY_CAR 3

using namespace model;

class AttackController
{
	double COUNTS_PER_TICK;
	double TIRE_SPEED_DECREASE_RATIO;

	AdditionalInfo* info;
	vector < pair <double, double> > carPositions;
	vector < pair <double, double> > carSpeeds;
	vector < bool > ignoredCars;
	vector < unsigned char > players;
	size_t carsCount;
	CarType myType;
	double trackTileSize, trackTileMargin;
	pair <double, double> projectilePosition;
	pair <double, double> projectileSpeed;
	int debug_i;

public:
	AttackController(AdditionalInfo* info_)
	{
		info = info_;
		COUNTS_PER_TICK = 10;
		TIRE_SPEED_DECREASE_RATIO = 0.5;
	}

	void init(const Car& self, const World& world, const Game& game)
	{
		myType = self.getType();
		carsCount = world.getCars().size();
		trackTileSize = game.getTrackTileSize();
		trackTileMargin = game.getTrackTileMargin();
		if (myType == BUGGY) {
			COUNTS_PER_TICK = 5;
		} else {
			COUNTS_PER_TICK = 10;
		}
		
		for (std::vector<Car>::const_iterator car_it = world.getCars().begin(); car_it != world.getCars().end(); ++car_it) {
			carPositions.push_back(make_pair(0,0));
			carSpeeds.push_back(make_pair(0,0));
			players.push_back(TEAMMATE_CAR);
			ignoredCars.push_back(
				(*car_it).getAngularSpeed() > 0.05 && (*car_it).getDistanceTo(self.getX(), self.getY()) > 400
				|| (*car_it).isFinishedTrack()		
			);
		}
	}

	double distance(pair <double, double> pos1, pair <double, double> pos2)
	{
		double dx = pos1.first - pos2.first;
		double dy = pos1.second - pos2.second;

		return sqrt(dx * dx + dy * dy);
	}

	void predictCarsNextTick(const World& world, const Game& game)
	{
		int carTilePosX, carTilePosY, carOffsetX, carOffsetY;
		for (int car_id = 0; car_id < carsCount; car_id++) {
			carTilePosX = carPositions[car_id].first / trackTileSize;
			carTilePosY = carPositions[car_id].second / trackTileSize;
			carOffsetX = (int) (carPositions[car_id].first + trackTileSize) % (int) trackTileSize;
			carOffsetY = (int) (carPositions[car_id].second + trackTileSize) % (int) trackTileSize;
			if (carTilePosX < 0 || carTilePosY < 0 || carOffsetX >= world.getTilesXY().size() || carOffsetY >= world.getTilesXY()[0].size()) {
				carSpeeds[car_id] = make_pair(0,0);
			}
			TileType type = world.getTilesXY()[carTilePosX][carTilePosY];
			double lowLimit = game.getTrackTileMargin() + game.getTireRadius();
			double highLimit = game.getTrackTileSize() - lowLimit;
			
			if (
				(carOffsetX >= lowLimit && carOffsetX <= highLimit && carOffsetX >= lowLimit && carOffsetX <= highLimit)
				|| (
					carOffsetX < lowLimit && (
						LEFT_HEADED_T == type
						|| RIGHT_BOTTOM_CORNER == type
						|| RIGHT_TOP_CORNER == type
						|| CROSSROADS == type
						|| HORIZONTAL == type
					)
				)
				|| (
					carOffsetX > highLimit && (
						RIGHT_HEADED_T == type
						|| LEFT_BOTTOM_CORNER == type
						|| LEFT_TOP_CORNER == type
						|| CROSSROADS == type
						|| HORIZONTAL == type
					)
				)
				|| (
					carOffsetY < lowLimit && (
						TOP_HEADED_T == type
						|| LEFT_BOTTOM_CORNER == type
						|| RIGHT_BOTTOM_CORNER == type
						|| CROSSROADS == type
						|| VERTICAL == type
					)
				)
				|| (
					carOffsetY > highLimit && (
						BOTTOM_HEADED_T == type
						|| LEFT_TOP_CORNER == type
						|| RIGHT_TOP_CORNER == type
						|| CROSSROADS == type
						|| VERTICAL == type
					)
				)
			) {
				carPositions[car_id].first += carSpeeds[car_id].first / COUNTS_PER_TICK;
				carPositions[car_id].second += carSpeeds[car_id].second / COUNTS_PER_TICK;
			} else {
				ignoredCars[car_id] = true;
			}
		}
	}

	void predictBulletNextTick(const World& world, const Game& game)
	{
		if (myType == BUGGY) {
			projectilePosition.first += projectileSpeed.first / COUNTS_PER_TICK;
			projectilePosition.second += projectileSpeed.second / COUNTS_PER_TICK;
		} else {
			int projectileTilePosX = projectilePosition.first / trackTileSize;
			int projectileTilePosY = projectilePosition.second / trackTileSize;
			int projectileOffsetX = (int) (projectilePosition.first + trackTileSize) % (int) trackTileSize;
			int projectileOffsetY = (int) (projectilePosition.second + trackTileSize) % (int) trackTileSize;
			if (projectileTilePosX < 0 || projectileTilePosY < 0 || projectileTilePosX >= world.getTilesXY().size() || projectileTilePosY >= world.getTilesXY()[0].size()) {
				projectileSpeed = make_pair(0,0);
			}
			TileType type = world.getTilesXY()[projectileTilePosX][projectileTilePosY];
			double lowLimit = game.getTrackTileMargin() + game.getTireRadius();
			double highLimit = game.getTrackTileSize() - lowLimit;

			double bushMinDistance = trackTileMargin + game.getTireRadius();

			// Клумбы
			if (
				(
					distance(make_pair(projectileOffsetX, projectileOffsetY), make_pair(0,0)) < bushMinDistance
					&& (
						type == CROSSROADS
						|| type == LEFT_HEADED_T
						|| type == TOP_HEADED_T
						|| type == RIGHT_BOTTOM_CORNER
					)
				)
				|| (
					distance(make_pair(projectileOffsetX, projectileOffsetY), make_pair(0, trackTileSize)) < bushMinDistance
					&& (
						type == CROSSROADS
						|| type == LEFT_HEADED_T
						|| type == BOTTOM_HEADED_T
						|| type == RIGHT_TOP_CORNER
					)
				)
				|| (
					distance(make_pair(projectileOffsetX, projectileOffsetY), make_pair(trackTileSize, 0)) < bushMinDistance
					&& (
						type == CROSSROADS
						|| type == RIGHT_HEADED_T
						|| type == TOP_HEADED_T
						|| type == LEFT_BOTTOM_CORNER
					)
				)
				|| (
					distance(make_pair(projectileOffsetX, projectileOffsetY), make_pair(trackTileSize, trackTileSize)) < bushMinDistance
					&& (
						type == CROSSROADS
						|| type == RIGHT_HEADED_T
						|| type == BOTTOM_HEADED_T
						|| type == LEFT_TOP_CORNER
					)
				)
			) {
				projectileSpeed.first = - TIRE_SPEED_DECREASE_RATIO * projectileSpeed.first;
				projectileSpeed.second = - TIRE_SPEED_DECREASE_RATIO * projectileSpeed.second;
				// Нужен нормальный просчет отскока
			} else {
				// Ox
				if (
					(
						projectileOffsetX <= lowLimit
						&& (
							type == VERTICAL
							|| type == RIGHT_HEADED_T
							|| type == LEFT_BOTTOM_CORNER
							|| type == LEFT_TOP_CORNER
						)	
					)
					|| (
						projectileOffsetX >= highLimit
						&& (
							type == VERTICAL
							|| type == LEFT_HEADED_T
							|| type == RIGHT_BOTTOM_CORNER
							|| type == RIGHT_TOP_CORNER
						)	
					)
				) {
					projectileSpeed.first = - TIRE_SPEED_DECREASE_RATIO * projectileSpeed.first;
				}

				// Oy
				if (
					(
						projectileOffsetY <= lowLimit
						&& (
							type == HORIZONTAL
							|| type == BOTTOM_HEADED_T
							|| type == LEFT_TOP_CORNER
							|| type == RIGHT_TOP_CORNER
						)	
					)
					|| (
						projectileOffsetY >= highLimit
						&& (
							type == HORIZONTAL
							|| type == TOP_HEADED_T
							|| type == LEFT_BOTTOM_CORNER
							|| type == RIGHT_BOTTOM_CORNER
						)	
					)
				) {
					projectileSpeed.second = - TIRE_SPEED_DECREASE_RATIO * projectileSpeed.second;
				}
			}

			projectilePosition.first += projectileSpeed.first / COUNTS_PER_TICK;
			projectilePosition.second += projectileSpeed.second / COUNTS_PER_TICK;
		}
	}

	void initCarPositionsAndSpeeds(const World& world)
	{
		int car_id = 0;
		for (std::vector<Car>::const_iterator car_it = world.getCars().begin(); car_it != world.getCars().end(); ++car_it, car_id++) {
			carPositions[car_id] = make_pair((*car_it).getX(), (*car_it).getY());
			carSpeeds[car_id] = make_pair((*car_it).getSpeedX(), (*car_it).getSpeedY());
		}
	}

	void initProjectileInfo(double x, double y, double speedX, double speedY)
	{
		projectilePosition = make_pair(x, y);
		projectileSpeed = make_pair(speedX, speedY);
	}
	
	unsigned char testInnerTireAttack(const Car& self, const World& world, const Game& game, double angle, double maxDistance)
	{
		unsigned char status = NONE;
		double expectedDist, initialSpeed;
		if (myType == BUGGY) {
			expectedDist = game.getWasherRadius();
			initialSpeed = game.getWasherInitialSpeed();
		} else {
			expectedDist = game.getTireRadius();
			initialSpeed = game.getTireInitialSpeed();
		}

		double minCarSize = self.getWidth() > self.getHeight() ? self.getHeight() : self.getWidth();
		expectedDist = expectedDist > minCarSize / 2 ? expectedDist : minCarSize / 2;
	
		initProjectileInfo(
			self.getX(),
			self.getY(),
			initialSpeed * cos(angle),
			initialSpeed * sin(angle)
		);

		initCarPositionsAndSpeeds(world);
		for (int i = 0; i < maxDistance * COUNTS_PER_TICK; i++) {
			debug_i = i;
			predictBulletNextTick(world, game);
			predictCarsNextTick(world, game);

			double fullProjectileSpeed = distance(projectileSpeed, make_pair(0,0));
			if (JEEP == myType && fullProjectileSpeed < game.getTireInitialSpeed() * game.getTireDisappearSpeedFactor()) {
				break;
			}

			
			int tick = 0;
			
			double myCarSpeed = distance(make_pair(self.getSpeedX(), self.getSpeedY()), make_pair(0, 0));
			double carMaxSize = self.getWidth() > self.getHeight() ? self.getWidth() : self.getHeight();
			double delta;
			if (BUGGY == myType) {
				delta = game.getWasherInitialSpeed() - myCarSpeed;
			} else {
				delta = game.getTireInitialSpeed() - myCarSpeed;
			}
			if (delta) {
				tick = (carMaxSize / 2) / delta;
			}
			
			int car_id = 0;
			for (std::vector<Car>::const_iterator car_it = world.getCars().begin(); car_it != world.getCars().end(); ++car_it, car_id++) {
				if (
					distance(projectilePosition, carPositions[car_id]) < expectedDist
					&& !ignoredCars[car_id]
					&& i > tick * COUNTS_PER_TICK
				) {
					double projectileTotalSpeed = distance(projectileSpeed, make_pair(0, 0));
					if (!(*car_it).getDurability() ||  JEEP == myType && projectileTotalSpeed < 0.5 * game.getTireInitialSpeed()) {
						status = NONE;
					} else {
						status = players[car_id];
					}
					
					i = maxDistance * COUNTS_PER_TICK;
					break;
				}
			}
		}

		return status;
	}

	bool getThrowProjectile(const Car& self, const World& world, const Game& game, double maxDistance)
	{
		int car_id = 0;
		for (std::vector<Car>::const_iterator car_it = world.getCars().begin(); car_it != world.getCars().end(); ++car_it, ++car_id) {
			if ((*car_it).getId() == self.getId()) {
				players[car_id] = MY_CAR;
			} else if ((*car_it).isTeammate()) {
				players[car_id] = TEAMMATE_CAR;
			} else {
				players[car_id] = ENEMY_CAR;
			}
		}

		bool status = false;
		if (self.getProjectileCount() && !self.getRemainingProjectileCooldownTicks()) {
			if (myType == BUGGY) {
				double dangle = PI / 90;
				unsigned char state1 = testInnerTireAttack(self, world, game, self.getAngle() - dangle, maxDistance);
				unsigned char state2 = testInnerTireAttack(self, world, game, self.getAngle(), maxDistance);
				unsigned char state3 = testInnerTireAttack(self, world, game, self.getAngle() + dangle, maxDistance);

				status = (ENEMY_CAR == state1 && ENEMY_CAR == state2 && (ENEMY_CAR == state3 || NONE == state3))
					|| ((ENEMY_CAR == state1 || NONE == state1) && ENEMY_CAR == state2 &&ENEMY_CAR == state3)
					|| (ENEMY_CAR == state1 && (ENEMY_CAR == state2 || NONE == state2) && ENEMY_CAR == state3);

				if(status) {
					int debug = 0;
				}
			} else {
				unsigned char state = testInnerTireAttack(self, world, game, self.getAngle(), maxDistance);
				status = (ENEMY_CAR == state);
			}
		}

		return status;
	}
};
#endif