"""Copyright (C) 2014 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
"""

############ Constants ############
# Difference between two ratings such that it is
# regarded as a "sure win" for the higher player.
# No points are gained or lost for such a game.
elo_sure_win_difference = 600.0

# Lower ratings "move faster" and change more
# dramatically than higher ones. Anything rating above
# this value moves at the same rate as this value.
elo_k_factor_constant_rating = 2200.0

# This preset number of games is the number of games
# where a player is considered "stable".
# Rating volatility is constant after this number.
volatility_constant = 20.0

# Fair rating adjustment loses against inflation
# This constant will battle inflation.
# NOTE: This can be adjusted as needed by a
# bot/server administrator
anti_inflation = 0.015

############ Functions ############
def get_rating_adjustment(rating, opponent_rating, games_played, opponent_games_played, result):
  """
    Calculates the rating adjustment after a 1v1 game finishes using simplified ELO.

    Arguments:
      rating, opponent_rating - Ratings of the players before this game.
      games_played, opponent_games_played - Number of games each player has played
        before this game.
      result - 1 for the first player (rating, games_played) won, 0 for draw, or
        -1 for the second player (opponent_rating, opponent_games_played) won.

    Returns:
      The integer that should be subtracted from the loser's rating and added
      to the winner's rating to get their new ratings.

    TODO: Team games.
  """
  player_volatility = (min(games_played, volatility_constant) / volatility_constant + 0.25) / 1.25
  rating_k_factor = 50.0 * (min(rating, elo_k_factor_constant_rating) / elo_k_factor_constant_rating + 1.0) / 2.0
  volatility = rating_k_factor * player_volatility
  difference = opponent_rating - rating
  if result == 1:
    return round(max(0, (difference + result * elo_sure_win_difference) / volatility - anti_inflation))
  elif result == -1:
    return round(min(0, (difference + result * elo_sure_win_difference) / volatility - anti_inflation))
  else:
    return round(difference / volatility - anti_inflation)

# Inflation test - A slightly negative is better than a slightly positive
# Lower rated players stop playing more often than higher rated players
# Uncomment to test.
# In this example, two evenly matched players play for 150000 games.
"""
from random import randrange
r1start = 1600
r2start = 1600
r1 = r1start
r2 = r2start
for x in range(0, 150000):
  res = randrange(3)-1 # How often one wins against the other
  if res >= 1:
    res = 1
  elif res <= -1:
    res = -1
  r1gain = get_rating_adjustment(r1, r2, 20, 20, res)
  r2gain = get_rating_adjustment(r2, r1, 20, 20, -1 * res)
  r1 += r1gain
  r2 += r2gain
print(str(r1) + " " + str(r2) + "   :   " + str(r1 + r2-r1start - r2start))
"""
