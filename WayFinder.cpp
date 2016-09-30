#ifndef WAY_FINDER
#define WAY_FINDER

#include "Strategy.h"
#include <cmath>
#include <cstdlib>
#include <deque>
#include <map>
#include <algorithm>

using namespace model;
using namespace std;

class WayFinder
{
	vector< vector<int> > traceMap;
	deque<pair<int,int> > queue;	// Очередь на обработку тайлов в BFS
	deque <pair<int,int> > realWayPoints;

	bool checkDown(TileType type)
	{
		return (
			type == VERTICAL
				|| type == LEFT_TOP_CORNER
				|| type == RIGHT_TOP_CORNER
				|| type == LEFT_HEADED_T
				|| type == RIGHT_HEADED_T
				|| type == BOTTOM_HEADED_T
				|| type == CROSSROADS
			);
	}

	bool checkUp(TileType type)
	{
		return (
			type == VERTICAL
				|| type == LEFT_BOTTOM_CORNER
				|| type == RIGHT_BOTTOM_CORNER
				|| type == LEFT_HEADED_T
				|| type == RIGHT_HEADED_T
				|| type == TOP_HEADED_T
				|| type == CROSSROADS
			);
	}

	bool checkLeft(TileType type)
	{
		return (
			type == HORIZONTAL
				|| type == RIGHT_TOP_CORNER
				|| type == RIGHT_BOTTOM_CORNER
				|| type == BOTTOM_HEADED_T
				|| type == LEFT_HEADED_T
				|| type == TOP_HEADED_T
				|| type == CROSSROADS
			);	
	}

	bool checkRight(TileType type)
	{
		return (
			type == HORIZONTAL
				|| type == LEFT_TOP_CORNER
				|| type == LEFT_BOTTOM_CORNER
				|| type == BOTTOM_HEADED_T
				|| type == RIGHT_HEADED_T
				|| type == TOP_HEADED_T
				|| type == CROSSROADS
			);	
	}

	void setMapTraceTileStatus(int tilePosX, int tilePosY, int value)
	{
		if (!traceMap[tilePosX][tilePosY]) {
			queue.push_back(make_pair(tilePosX, tilePosY));
			traceMap[tilePosX][tilePosY] = value + 1;
		}
	}

	void trace2SingleWaypoint(
		const World& world,
		pair<int,int> currentPos,
		pair<int,int> nextWayPoint,
		const Car& self,
		const Game& game,
		Direction currentDirection
		)
	{
		queue.clear();
		for (int i = 0; i < traceMap.size(); i++) for (int j = 0; j < traceMap[i].size(); j++) traceMap[i][j] = 0;
		traceMap[currentPos.first][currentPos.second] = 1;
		queue.push_back(currentPos);

		TileType type;
		pair<int,int> current;
		int map_width = world.getTilesXY().size();
		int map_height = world.getTilesXY()[0].size();
		int iteration;
		while (queue.size() > 0) {
			current = queue.front();
			queue.pop_front();
			type = world.getTilesXY()[current.first][current.second];
			iteration = traceMap[current.first][current.second];
			if (current == nextWayPoint) {
				queue.clear();
				continue;
			}

			if ((current != currentPos || current == currentPos && currentDirection != DOWN) && current.second > 0 && checkUp(type)) {
				setMapTraceTileStatus(current.first, current.second - 1, iteration);
			}

			if ((current != currentPos || current == currentPos && currentDirection != UP) && current.second < map_height - 1 && checkDown(type)) {
				setMapTraceTileStatus(current.first, current.second + 1, iteration);
			}

			if ((current != currentPos || current == currentPos && currentDirection != RIGHT) && current.first > 0 && checkLeft(type)) {
				setMapTraceTileStatus(current.first - 1, current.second, iteration);
			}

			if ((current != currentPos || current == currentPos && currentDirection != LEFT) && current.first < map_width - 1 && checkRight(type)) {
				setMapTraceTileStatus(current.first + 1, current.second, iteration);	
			}
		}

		int waypointTileId =  traceMap[nextWayPoint.first][nextWayPoint.second];
		realWayPoints.push_front(make_pair(nextWayPoint.first, nextWayPoint.second));

		current = nextWayPoint;
		for (int i = waypointTileId - 1; i > 1; i--) {
			type = world.getTilesXY()[current.first][current.second];
			if (current.second > 0 && traceMap[current.first][current.second - 1] == i && checkUp(type)) {
				current.second -= 1;
				realWayPoints.push_front(current);
			} else if (current.second < map_height - 1 && traceMap[current.first][current.second + 1] == i && checkDown(type)) {
				current.second += 1;
				realWayPoints.push_front(current);
			} else if (current.first > 0 && traceMap[current.first - 1][current.second] == i && checkLeft(type)) {
				current.first -= 1;
				realWayPoints.push_front(current);
			} else if (current.first < map_width - 1 && traceMap[current.first + 1][current.second] == i && checkRight(type)) {
				current.first += 1;
				realWayPoints.push_front(current);
			}
		}
	}

public:
	WayFinder()
	{
		
	}

	void init(const World& world)
	{
		queue.clear();
		int map_width = world.getTilesXY().size();
		int map_height = world.getTilesXY()[0].size();
		for (int x = 0; x < map_width; x++) {
			traceMap.push_back(vector<int>());
			for (int y = 0; y < map_height; y++) {
				traceMap[x].push_back(0);
			}
		}
	}

	deque <pair<int,int> > trace(const World& world, pair<int,int> currentPos, const Car& self, const Game& game, Direction currentDirection)
	{
		vector<int> nextNextWayPoint = world.getWaypoints()[(self.getNextWaypointIndex() + 1) % world.getWaypoints().size()];
		vector<int> nextNextNextWayPoint = world.getWaypoints()[(self.getNextWaypointIndex() + 2) % world.getWaypoints().size()];
		pair<int,int> nextWayPoint = make_pair(self.getNextWaypointX(), self.getNextWaypointY());

		realWayPoints.clear();
		trace2SingleWaypoint(
			world,
			currentPos,
			make_pair(self.getNextWaypointX(), self.getNextWaypointY()),
			self,
			game,
			currentDirection
		);
		if (!realWayPoints.size()) {
			currentDirection = _UNKNOWN_DIRECTION_;
		}
		realWayPoints.clear();

		trace2SingleWaypoint(
			world,
			make_pair(nextNextWayPoint[0], nextNextWayPoint[1]),
			make_pair(nextNextNextWayPoint[0], nextNextNextWayPoint[1]),
			self,
			game,
			_UNKNOWN_DIRECTION_
		);

		trace2SingleWaypoint(
			world,
			make_pair(self.getNextWaypointX(), self.getNextWaypointY()),
			make_pair(nextNextWayPoint[0], nextNextWayPoint[1]),
			self,
			game,
			_UNKNOWN_DIRECTION_
		);

		trace2SingleWaypoint(
			world,
			currentPos,
			make_pair(self.getNextWaypointX(), self.getNextWaypointY()),
			self,
			game,
			currentDirection
		);

		return realWayPoints;
	}
};
#endif