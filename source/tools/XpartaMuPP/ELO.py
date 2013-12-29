"""Copyright (C) 2013 Wildfire Games.
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
elo_sure_win_difference = 600

# Lower ratings "move faster" and change more
# dramatically than higher ones. Anything rating above
# this value moves at the same rate as this value.
elo_k_factor_constant_rating = 2200

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

  opponent_volatility_influence = max(1, pow(min(games_played + 1, 50) / min(opponent_games_played + 1, 50), 0.5))
  rating_k_factor = 0.75 * pow(elo_k_factor_constant_rating / min(elo_k_factor_constant_rating, (rating + opponent_rating) / 2), 0.5)
  player_volatility = min(pow(1.1, games_played + 16), 25)
  volatility = opponent_volatility_influence * player_volatility / rating_k_factor
  difference = opponent_rating - rating
  if result == 1:
    return round(max(0, (difference + result * elo_sure_win_difference) / volatility))
  elif result == -1:
    return round(min(0, (difference + result * elo_sure_win_difference) / volatility))
  else:
    return round(difference / volatility)
