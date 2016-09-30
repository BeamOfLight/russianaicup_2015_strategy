#include "MyStrategy.h"

//Iguana v4

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES

#include <cmath>
#include <cstdlib>
#include <deque>
#include <algorithm>
#include "AdditionalInfo.cpp"
#include "AttackController.cpp"

using namespace model;
using namespace std;

float WHEEL_RATIO = 3;
float BEAM_STRAT_CORNER_OFFSET = 0.19;
float BEAM_STRAT_PRE_TURN_OFFSET = 0.29;
float BEAM_STRAT_ENGINE_POWER = 1.0;

float BEAM_STRAT_SECOND_CORNER_SPEED = 11;
float BEAM_STRAT_CORNER_SPEED_1 = 17;
float BEAM_STRAT_CORNER_SPEED_2 = 31;

int BEAM_STRAT_MAX_TICKS_WITHOUT_SPEED = 50;
float BEAM_STRAT_STRAIGHT_TILES_FOR_NITRO = 5;
int BEAM_STRAT_START_TIME_WITHOUT_NITRO = 280;

int BEAM_STRAT_PRE_TURN_PREPARE_TILES = 3;
int BEAM_STRAT_BONUS_DISABLE_TILES = 1;
int BEAM_STRAT_MAX_ATTACK_DISTANCE = 80; // * 60

int BEAM_STRAT_REVERSE_MODE_ACTIVE_TIME[4] = {120, 140, 100, 90};
int BEAM_STRAT_REVERSE_MODE_PASSIVE_TIME = 20;
float BEAM_STRAT_REVERSE_MODE_ENGINE_POWER = 1;

int countTicksWithoutSpeed = 0;
int countTicksForReverse = 0;
bool reverseMode = false;
int reverseModeCount = 0;
int reverseModeType = 0;
float savedWheelTurn;
unsigned char currentDirection1, currentDirection2;

AdditionalInfo* info = new AdditionalInfo();
AttackController* attackController = new AttackController(info);

bool checkBonusOnMyWay(Bonus bonus, const Game& game)
{
	pair<int, int> bonusTilePos = make_pair(
		bonus.getX()/game.getTrackTileSize(),
		bonus.getY()/game.getTrackTileSize()
	);

	bool status = false;
	deque <pair<int,int> > realWayPoints = info->getRealWayPoints();
	deque <pair<int,int> >::iterator end = realWayPoints.begin() + ((realWayPoints.size() > 3) ? 3 : realWayPoints.size());
	deque <pair<int,int> >::iterator it = std::find(realWayPoints.begin(), end, bonusTilePos); 
	if (it != end) {
		status = true;
	}

	return status;
}

bool checkOilSticksOnMyWay(const World& world, const Game& game)
{
	bool status = false;
	deque <pair<int,int> > realWayPoints = info->getRealWayPoints();
	deque <pair<int,int> >::iterator end = realWayPoints.begin() + ((realWayPoints.size() > BEAM_STRAT_STRAIGHT_TILES_FOR_NITRO) ? BEAM_STRAT_STRAIGHT_TILES_FOR_NITRO : realWayPoints.size());
	deque <pair<int,int> >::iterator it;
	pair<int,int> oilStickTilePos;
	for (vector<OilSlick>::const_iterator oilStickIt = world.getOilSlicks().begin(); oilStickIt !=  world.getOilSlicks().end(); ++oilStickIt) {
		oilStickTilePos = make_pair(
			(*oilStickIt).getX() /game.getTrackTileSize(),
			(*oilStickIt).getY() /game.getTrackTileSize()
		);
		it = std::find(realWayPoints.begin(), end, oilStickTilePos); 
		if (it != end) {
			status = true;
			break;
		}
	}

	return status;
}

bool check3CurrentDirections(Direction direction0, Direction direction1, Direction direction2)
{
	return (
		info->getCurrentDirection(0) == direction0
		&& info->getCurrentDirection(1) == direction1
		&& info->getCurrentDirection(2) == direction2
	);
}

bool check4CurrentDirections(Direction direction0, Direction direction1, Direction direction2, Direction direction3)
{
	return (
		info->getCurrentDirection(0) == direction0
		&& info->getCurrentDirection(1) == direction1
		&& info->getCurrentDirection(2) == direction2
		&& info->getCurrentDirection(3) == direction3
	);
}

bool checkLastAnd2CurrentDirections(Direction lastDirection, Direction direction0, Direction direction1)
{
	return (
		info->getLastDirection() == lastDirection
		&& info->getCurrentDirection(0) == direction0
		&& info->getCurrentDirection(1) == direction1
	);
}

bool checkLastAnd3CurrentDirections(Direction lastDirection, Direction direction0, Direction direction1, Direction direction2)
{
	return (
		info->getLastDirection() == lastDirection
		&& info->getCurrentDirection(0) == direction0
		&& info->getCurrentDirection(1) == direction1
		&& info->getCurrentDirection(2) == direction2
	);
}

bool isDiagonalRoad()
{
	return (
		checkLastAnd3CurrentDirections(RIGHT, UP, RIGHT, UP) || checkLastAnd3CurrentDirections(UP, RIGHT, UP, RIGHT)
		|| checkLastAnd3CurrentDirections(LEFT, UP, LEFT, UP) || checkLastAnd3CurrentDirections(UP, LEFT, UP, LEFT)
		|| checkLastAnd3CurrentDirections(RIGHT, DOWN, RIGHT, DOWN) || checkLastAnd3CurrentDirections(DOWN, RIGHT, DOWN, RIGHT)
		|| checkLastAnd3CurrentDirections(LEFT, DOWN, LEFT, DOWN) || checkLastAnd3CurrentDirections(DOWN, LEFT, DOWN, LEFT)
	);
}



bool isDificultTurn()
{
	return (
		check3CurrentDirections(LEFT, UP, RIGHT) || checkLastAnd2CurrentDirections(LEFT, UP, RIGHT)
		|| check3CurrentDirections(LEFT, DOWN, RIGHT) || checkLastAnd2CurrentDirections(LEFT, DOWN, RIGHT)
		|| check3CurrentDirections(RIGHT, UP, LEFT) || checkLastAnd2CurrentDirections(RIGHT, UP, LEFT)
		|| check3CurrentDirections(RIGHT, DOWN, LEFT) || checkLastAnd2CurrentDirections(RIGHT, DOWN, LEFT)
		|| check3CurrentDirections(UP, LEFT, DOWN) || checkLastAnd2CurrentDirections(UP, LEFT, DOWN)
		|| check3CurrentDirections(UP, RIGHT, DOWN) || checkLastAnd2CurrentDirections(UP, RIGHT, DOWN)
		|| check3CurrentDirections(DOWN, RIGHT, UP) || checkLastAnd2CurrentDirections(DOWN, RIGHT, UP)
		|| check3CurrentDirections(DOWN, LEFT, UP) || checkLastAnd2CurrentDirections(DOWN, LEFT, UP)
	);
}

bool isDificultTurn2()
{
	return (
		check4CurrentDirections(UP, UP, LEFT, UP)
		|| check4CurrentDirections(UP, UP, RIGHT, UP)
		|| check4CurrentDirections(DOWN, DOWN, LEFT, DOWN)
		|| check4CurrentDirections(DOWN, DOWN, RIGHT, DOWN)
		|| check4CurrentDirections(LEFT, LEFT, UP, LEFT)
		|| check4CurrentDirections(LEFT, LEFT, DOWN, LEFT)
		|| check4CurrentDirections(RIGHT, RIGHT, UP, RIGHT)
		|| check4CurrentDirections(RIGHT, RIGHT, DOWN, RIGHT)
	);
}

void MyStrategy::move(const Car& self, const World& world, const Game& game, Move& move)
{
	if (info->needInit()) {
		info->init(self, world, game);
		attackController->init(self, world, game);
	}

	if (world.getProjectiles().size()) {
		int debug = 0;
	}
	move.setEnginePower(BEAM_STRAT_ENGINE_POWER);
    if (world.getTick() > game.getInitialFreezeDurationTicks() && !self.isFinishedTrack()) {
		info->update(self, world, game, move);
		Direction currentDirection1 = info->getCurrentDirection(0);
		Direction currentDirection2 = info->getCurrentDirection(1);

		if (reverseMode) {
			if (countTicksWithoutSpeed > BEAM_STRAT_REVERSE_MODE_PASSIVE_TIME) {
				move.setWheelTurn(-savedWheelTurn);
				move.setEnginePower(-BEAM_STRAT_ENGINE_POWER);
			} else {
				move.setWheelTurn(0);
				move.setBrake(true);
			}
			
			countTicksWithoutSpeed--;
			if (countTicksWithoutSpeed <= 0) {
				info->trace(world, info->getCurrentTilePos(), self, game);
				currentDirection1 = info->getCurrentDirection(0);
				currentDirection2 = info->getCurrentDirection(1);
				reverseMode = false;
				move.setWheelTurn(savedWheelTurn);
			}
		} else {
			// Кол-во тайлов до поворота
			int tilesBeforeTurn = info->countTilesBeforeTurn();

			//NITRO
			if (
				tilesBeforeTurn > BEAM_STRAT_STRAIGHT_TILES_FOR_NITRO
				&& world.getTick() > BEAM_STRAT_START_TIME_WITHOUT_NITRO
				&& !checkOilSticksOnMyWay(world, game)
				&& !info->isAfterTurnNow()
				) {
				move.setUseNitro(true);
			}

			// Повороты
			bool bonusMode = false;
			pair<int,int> resultBonusPosition;
			float waypoint_offset_x = 0.5;
			float waypoint_offset_y = 0.5;

			pair<int,int> straightDirectionPos;
			if (currentDirection1 == UP) {
				straightDirectionPos = make_pair(self.getX(), self.getY() - game.getTrackTileSize());
			} else if (currentDirection1 == DOWN) {
				straightDirectionPos = make_pair(self.getX(), self.getY() + game.getTrackTileSize());
			} else if (currentDirection1 == LEFT) {
				straightDirectionPos = make_pair(self.getX() - game.getTrackTileSize(), self.getY());
			} else if (currentDirection1 == RIGHT) {
				straightDirectionPos = make_pair(self.getX() + game.getTrackTileSize(), self.getY());
			}

			if (currentDirection1 == _UNKNOWN_DIRECTION_
				|| currentDirection2 == _UNKNOWN_DIRECTION_
				|| isDificultTurn2()
			) {
				waypoint_offset_x = 0.5;
				waypoint_offset_y = 0.5;
			} else if (isDiagonalRoad()) {
				if ((info->getCurrentDirection(0) == UP || info->getCurrentDirection(0) == DOWN) && info->getCurrentDirection(1) == RIGHT) {
					waypoint_offset_x = 1;
					waypoint_offset_y = 0.5;
				} else if ((info->getCurrentDirection(0) == LEFT || info->getCurrentDirection(0) == RIGHT) && info->getCurrentDirection(1) == UP) {
					waypoint_offset_x = 0.5;
					waypoint_offset_y = 0;
				} else if ((info->getCurrentDirection(0) == UP || info->getCurrentDirection(0) == DOWN) && info->getCurrentDirection(1) == LEFT) {
					waypoint_offset_x = 0;
					waypoint_offset_y = 0.5;
				} else if ((info->getCurrentDirection(0) == LEFT || info->getCurrentDirection(0) == RIGHT) && info->getCurrentDirection(1) == DOWN) {
					waypoint_offset_x = 0.5;
					waypoint_offset_y = 1;
				}
			} else if (currentDirection1 == UP && currentDirection2 == RIGHT || currentDirection1 == LEFT && currentDirection2 == DOWN) {
				waypoint_offset_x = 1 - BEAM_STRAT_CORNER_OFFSET;
				waypoint_offset_y = 1 - BEAM_STRAT_CORNER_OFFSET;
			} else if (currentDirection1 == UP && currentDirection2 == LEFT || currentDirection1 == RIGHT && currentDirection2 == DOWN) {
				waypoint_offset_x = BEAM_STRAT_CORNER_OFFSET;
				waypoint_offset_y = 1 - BEAM_STRAT_CORNER_OFFSET;
			} else if (currentDirection1 == RIGHT && currentDirection2 == UP || currentDirection1 == DOWN && currentDirection2 == LEFT) {
				waypoint_offset_x = BEAM_STRAT_CORNER_OFFSET;
				waypoint_offset_y = BEAM_STRAT_CORNER_OFFSET;
			} else if (currentDirection1 == LEFT && currentDirection2 == UP || currentDirection1 == DOWN && currentDirection2 == RIGHT) {
				waypoint_offset_x = 1 - BEAM_STRAT_CORNER_OFFSET;
				waypoint_offset_y = BEAM_STRAT_CORNER_OFFSET;
			} else if (tilesBeforeTurn <= BEAM_STRAT_PRE_TURN_PREPARE_TILES && (currentDirection1 == UP || currentDirection1 == DOWN)) {
				waypoint_offset_x = ((float) (self.getX() - info->getCurrentTilePos().first * game.getTrackTileSize()))/game.getTrackTileSize();
				waypoint_offset_y = 0.5;
				if (info->getTurnDirection() == RIGHT && (currentDirection1 == UP || currentDirection1 == DOWN)) {
					waypoint_offset_x = BEAM_STRAT_PRE_TURN_OFFSET;
				} else if (info->getTurnDirection() == LEFT && (currentDirection1 == UP || currentDirection1 == DOWN)) {
					waypoint_offset_x = 1 - BEAM_STRAT_PRE_TURN_OFFSET;
				}
			} else if (tilesBeforeTurn <= BEAM_STRAT_PRE_TURN_PREPARE_TILES && (currentDirection1 == LEFT || currentDirection1 == RIGHT)) {
				waypoint_offset_x = 0.5;
				waypoint_offset_y = ((float) (self.getY() - info->getCurrentTilePos().second * game.getTrackTileSize()))/game.getTrackTileSize();
				if (info->getTurnDirection() == UP && (currentDirection1 == LEFT || currentDirection1 == RIGHT)) {
					waypoint_offset_y = 1 - BEAM_STRAT_PRE_TURN_OFFSET;
				} else if (info->getTurnDirection() == DOWN && (currentDirection1 == LEFT || currentDirection1 == RIGHT)) {
					waypoint_offset_y = BEAM_STRAT_PRE_TURN_OFFSET;
				}
			} else if (fabs(self.getAngleTo(straightDirectionPos.first, straightDirectionPos.second)) < PI / 9 && currentDirection1 == currentDirection2 && tilesBeforeTurn >= BEAM_STRAT_BONUS_DISABLE_TILES) {
				float angle_to_bonus, dist_to_bonus;
				vector<Bonus>::const_iterator target_bonus_it = world.getBonuses().end();
				float target_dist = world.getTilesXY().size() * game.getTrackTileSize();

				for (vector<Bonus>::const_iterator bonus_it = world.getBonuses().begin(); bonus_it != world.getBonuses().end(); ++bonus_it) {
					angle_to_bonus = self.getAngleTo((*bonus_it).getX(), (*bonus_it).getY());
					dist_to_bonus = self.getDistanceTo((*bonus_it).getX(), (*bonus_it).getY());
					if (
						(
							fabs(angle_to_bonus) < PI / 8 && info->getSpeedLevel() == 1
							|| fabs(angle_to_bonus) < PI / 10 && info->getSpeedLevel() == 2
							|| fabs(angle_to_bonus) < PI / 12 && info->getSpeedLevel() == 3
						)
						&& dist_to_bonus < target_dist && checkBonusOnMyWay(*bonus_it, game)
						|| dist_to_bonus < game.getTrackTileSize() && fabs(angle_to_bonus) < PI / 16 && dist_to_bonus < target_dist
						) {
						target_dist = dist_to_bonus;
						target_bonus_it = bonus_it;
					}
				}

				if (target_bonus_it != world.getBonuses().end() && target_dist < 3 * game.getTrackTileSize()) {
					bonusMode = true;
					pair<int,int> bonusPosition = make_pair((*target_bonus_it).getX(), (*target_bonus_it).getY());
					int intersect_val = 2;
					int dx = (*target_bonus_it).getWidth() / 2;
					int dy = (*target_bonus_it).getHeight() / 2;
					vector < pair<int,int> > bonusCorners;
					bonusCorners.push_back(bonusPosition);
					pair<int,int> bonusPositionInTile = make_pair(bonusPosition.first % (int)(game.getTrackTileSize()), bonusPosition.second % (int)(game.getTrackTileSize()));
					pair<float,float> relativeBonusPosition = make_pair(bonusPositionInTile.first / game.getTrackTileSize(), bonusPositionInTile.second / game.getTrackTileSize());
					if (currentDirection1 == UP || currentDirection1 == DOWN) {
						if (bonusPositionInTile.first > 0.8) {
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second + dy));
						} else if (bonusPositionInTile.first < 0.2) {
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second + dy));
						} else {
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second + dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second + dy));
						}
					} else if (currentDirection1 == LEFT || currentDirection1 == RIGHT) {
						if (bonusPositionInTile.second > 0.8) {
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second - dy));
						} else if (bonusPositionInTile.second < 0.2) {
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second + dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second + dy));
						} else {
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second - dy));
							bonusCorners.push_back(make_pair(bonusPosition.first - dx, bonusPosition.second + dy));
							bonusCorners.push_back(make_pair(bonusPosition.first + dx, bonusPosition.second + dy));
						}
					}
					
					vector <float> bonusAnglesTo;
					float minAngle = PI / 2;
					float currentAngleTo;
					resultBonusPosition = bonusPosition;
					for (vector < pair<int,int> >::iterator corner_it = bonusCorners.begin(); corner_it != bonusCorners.end(); ++corner_it) {
						currentAngleTo = self.getAngleTo((*corner_it).first, (*corner_it).second);
						if (fabs(minAngle) > fabs(currentAngleTo)) {
							minAngle = currentAngleTo;
							resultBonusPosition = *corner_it;
						}
					}
				}
			}
			int waypoint_x, waypoint_y;
			if (bonusMode) {
				waypoint_x = resultBonusPosition.first;
				waypoint_y = resultBonusPosition.second;
			} else {
				waypoint_x = (info->getNextRealWaypoint().first + waypoint_offset_x) * game.getTrackTileSize();
				waypoint_y = (info->getNextRealWaypoint().second + waypoint_offset_y) * game.getTrackTileSize();
			}
			
			double dist = self.getDistanceTo(waypoint_x, waypoint_y);
			if (
				!isDiagonalRoad()
				&& (
					tilesBeforeTurn <= 1 && !isDificultTurn2() && info->getSpeed() > BEAM_STRAT_CORNER_SPEED_1
					|| tilesBeforeTurn <= 2 && info->getSpeed() > BEAM_STRAT_CORNER_SPEED_2
					|| isDificultTurn() && info->getSpeed() > BEAM_STRAT_SECOND_CORNER_SPEED
				)
			) {
				move.setBrake(true);
				move.setUseNitro(false);
			}			
			
			int pos_tile_x, pos_tile_y;
			deque < pair<int, int> > tilesHistory = info->getTilesHistory();
			if (self.getOilCanisterCount() && info->isAfterTurnNow()) {
				for (std::vector<Car>::const_iterator car_it = world.getCars().begin(); car_it != world.getCars().end(); ++car_it) {
					if ((*car_it).isTeammate() || (*car_it).isFinishedTrack()) continue;
					pos_tile_x = (*car_it).getX()/game.getTrackTileSize();
					pos_tile_y = (*car_it).getY()/game.getTrackTileSize();
					for (deque < pair <int,int> >::iterator hist_it = tilesHistory.begin(); hist_it != tilesHistory.end(); ++hist_it) {
						if ((*hist_it).first == pos_tile_x && (*hist_it).second == pos_tile_y && hist_it != tilesHistory.begin()) move.setSpillOil(true);
					}
				}
			}
			
			double angle_to = self.getAngleTo(waypoint_x, waypoint_y);
			double wheel_turn = WHEEL_RATIO * angle_to;
			if (fabs(wheel_turn) > 1) {
				wheel_turn = wheel_turn / fabs(wheel_turn);
			}
			
			move.setWheelTurn(wheel_turn);
			
			if (self.getDurability() > 0 && info->getSpeed() < 0.5 && self.getEnginePower() > 0.95) {
				countTicksWithoutSpeed++;
				if (countTicksWithoutSpeed > BEAM_STRAT_MAX_TICKS_WITHOUT_SPEED) {
					countTicksWithoutSpeed = BEAM_STRAT_REVERSE_MODE_ACTIVE_TIME[reverseModeType % 4] + BEAM_STRAT_REVERSE_MODE_PASSIVE_TIME;
					reverseMode = true;
					reverseModeCount++;
					if (reverseModeCount > 3) {
						reverseModeType++;
					}
					savedWheelTurn = wheel_turn;
				}
			} else {
				countTicksWithoutSpeed = 0;
			}

			move.setThrowProjectile(
				attackController->getThrowProjectile(self, world, game, BEAM_STRAT_MAX_ATTACK_DISTANCE)
			);
		}
		info->postUpdate(world);
	}
}

MyStrategy::MyStrategy()
{

}