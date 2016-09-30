#ifndef ADDITIONAL_INFO
#define ADDITIONAL_INFO

#include "Strategy.h"
#include <cmath>
#include <cstdlib>
#include <deque>
#include <map>
#include <algorithm>
#include "WayFinder.cpp"


using namespace model;
using namespace std;

class AdditionalInfo
{
	pair<int,int> currentTilePos;
	pair<int, int> lastMoveTilePos;
	bool needInit_;

	WayFinder* wayFinder;
	deque < pair<int, int> > realWayPoints;
	deque < pair<int, int> > tilesHistory;
	map<string, int> intConfig;
	vector < Direction > currentDirections;
	pair <int, int> nextRealWaypoint;
	float speed;

	//Следим за всеми машинами
	vector < pair <double, double> > lastMoveCarSpeeds;

public:
	AdditionalInfo()
	{
		wayFinder = new WayFinder();
		intConfig["tiles_history_limit"] = 3;
		needInit_ = true;
	}

	void updateTilePos(const Car& self, const Game& game)
	{
		currentTilePos = make_pair(self.getX()/game.getTrackTileSize(), self.getY()/game.getTrackTileSize());
	}

	void init(const Car& self, const World& world, const Game& game)
	{
		updateTilePos(self, game);
		lastMoveTilePos = currentTilePos;
		wayFinder->init(world);
		trace(world, currentTilePos, self, game);
		needInit_ = false;
		speed = 0;
		for (int i = 0; i < world.getCars().size(); i++) lastMoveCarSpeeds.push_back(make_pair(0.0,0.0));
	}

	Direction getCurrentDirection(pair<int,int> startPos, pair<int,int> endPos)
	{
		Direction direction = _UNKNOWN_DIRECTION_;
		if (startPos.first == endPos.first && startPos.second == endPos.second + 1) {
			direction = UP;
		} else if (startPos.first == endPos.first && startPos.second == endPos.second - 1) {
			direction = DOWN;
		}  else if (startPos.first == endPos.first - 1 && startPos.second == endPos.second) {
			direction = RIGHT;
		} else if (startPos.first == endPos.first + 1 && startPos.second == endPos.second) {
			direction = LEFT;
		}

		return direction;
	}

	void updateCurrentDirections()
	{
		currentDirections.clear();
		currentDirections.push_back(getCurrentDirection(currentTilePos, realWayPoints.front()));
		for (int i = 1; i < realWayPoints.size(); i++) {
			currentDirections.push_back(getCurrentDirection(realWayPoints[i-1], realWayPoints[i]));
		}
	}

	void update(const Car& self, const World& world, const Game& game, Move& move)
	{
		updateTilePos(self, game);
		if (currentTilePos != lastMoveTilePos) {
			lastMoveTilePos = currentTilePos;
			trace(world, currentTilePos, self, game);
		}

		if (!tilesHistory.size() || tilesHistory.size() && tilesHistory.front() != currentTilePos) {
			tilesHistory.push_front(currentTilePos);
			if (tilesHistory.size() > max(1, intConfig["tiles_history_limit"])) {
				tilesHistory.pop_back();
			}
		}

		speed = sqrt(self.getSpeedX() * self.getSpeedX() + self.getSpeedY() * self.getSpeedY());
	}

	void postUpdate(const World& world)
	{
		for (int i = 0; i < world.getCars().size(); i++) {
			lastMoveCarSpeeds[i] = make_pair(world.getCars()[i].getSpeedX(), world.getCars()[i].getSpeedY());
		}
	}


	vector < pair <double, double> >getLastMoveCarSpeeds()
	{
		return lastMoveCarSpeeds;
	}
	
	pair<int,int> getCurrentTilePos()
	{
		return currentTilePos;
	}

	pair<int,int> getLastMoveTilePos()
	{
		return lastMoveTilePos;
	}

	deque < pair<int, int> > getTilesHistory()
	{
		return tilesHistory;
	}

	void trace(const World& world, pair<int,int> currentPos, const Car& self, const Game& game)
	{
		realWayPoints = wayFinder->trace(world, currentPos, self, game, getCurrentDirection(0));
		nextRealWaypoint = realWayPoints.front();
		updateCurrentDirections();
	}

	pair <int, int> getNextRealWaypoint()
	{
		return realWayPoints.front();
	}

	Direction getLastDirection()
	{
		Direction value = _UNKNOWN_DIRECTION_;
		if (tilesHistory.size() > 1) {
			value = getCurrentDirection(tilesHistory[1], tilesHistory.front());
		}

		return value;
	}

	Direction getCurrentDirection(size_t pos)
	{
		Direction value = _UNKNOWN_DIRECTION_;
		if (pos >= 0 && pos < currentDirections.size()) {
			value = currentDirections[pos];
		}

		return value;
	}

	size_t countTilesBeforeTurn()
	{
		int cnt = 0;
		for (vector < Direction >::iterator it = currentDirections.begin(); it != currentDirections.end(); ++it) {
			if ((*it) == currentDirections.front()) {
				cnt++;
			} else {
				break;
			}
		}

		return cnt;
	}

	Direction getTurnDirection()
	{
		Direction turnDirection = _UNKNOWN_DIRECTION_;
		for (vector < Direction >::iterator it = currentDirections.begin(); it != currentDirections.end(); ++it) {
			if (*it != currentDirections.front()) {
				turnDirection = *it;
				break;
			}
		}

		return turnDirection;
	}

	deque < pair<int,int> > getRealWayPoints()
	{
		return realWayPoints;
	}

	bool needInit()
	{
		return needInit_;
	}

	bool isAfterTurnNow()
	{
		bool status = false;
		if (tilesHistory.size() > 2) {
			status = getCurrentDirection(tilesHistory[2], tilesHistory[1]) != getCurrentDirection(tilesHistory[1], tilesHistory.front());
		}

		return status;
	}

	float getSpeed()
	{
		return speed;
	}

	int getSpeedLevel()
	{
		int level = 0;
		if (speed > 0 && speed < 20) {
			level = 1;
		} else if (speed >= 20 && speed < 30) {
			level = 2;
		} else if (speed >= 30) {
			level = 3;
		}

		return level;
	}
};
#endif