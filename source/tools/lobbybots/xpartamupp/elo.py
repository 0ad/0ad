# Copyright (C) 2021 Wildfire Games.
# This file is part of 0 A.D.
#
# 0 A.D. is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# 0 A.D. is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.

"""Implementation of the ELO-rating algorithm for 0ad games."""

# Difference between two ratings such that it is regarded as a "sure
# win" for the higher player. No points are gained or lost for such a
# game.
ELO_SURE_WIN_DIFFERENCE = 600

# Lower ratings "move faster" and change more
# dramatically than higher ones. Anything rating above
# this value moves at the same rate as this value.
ELO_K_FACTOR_CONSTANT_RATING = 2200

# This preset number of games is the number of games where a player is
# considered "stable". Rating volatility is constant after this number.
VOLATILITY_CONSTANT = 20

# Fair rating adjustment loses against inflation.
# This constant will battle inflation.
# NOTE: This can be adjusted as needed by a bot/server administrator
ANTI_INFLATION = 0.015


def get_rating_adjustment(rating, opponent_rating, games_played,
                          opponent_games_played, result):  # pylint: disable=unused-argument
    """Calculate the rating adjustment after rated 1v1 games.

     The rating adjustment is calculated using a simplified
     ELO-algorithm.

     The given implementation doesn't work for negative ratings below
     -2199. This is a known limitation which is currently considered
     to be not relevant in day-to-day use.

    Arguments:
        rating (int): Rating of the first player before the game.
        opponent_rating (int): Rating of the second player before the
                               game.
        games_played (int): Number of games the first player has played
                            before this game.
        opponent_games_played (int): Number of games the second player
                                     has played before this game.
        result (int): 1 if the first player won, 0 if draw or -1 if the
                      second player won.

    Returns:
        int: the adjustment which should be applied to the rating of
             the first player

    """
    if rating < -2199 or opponent_rating < -2199:
        raise ValueError('Too small rating given: rating: %i, opponent rating: %i' %
                         (rating, opponent_rating))

    rating_k_factor = 50.0 * (min(rating, ELO_K_FACTOR_CONSTANT_RATING) /
                              ELO_K_FACTOR_CONSTANT_RATING + 1.0) / 2.0
    player_volatility = (min(max(0, games_played), VOLATILITY_CONSTANT) /
                         VOLATILITY_CONSTANT + 0.25) / 1.25
    volatility = rating_k_factor * player_volatility
    rating_difference = opponent_rating - rating
    rating_adjustment = (rating_difference + result * ELO_SURE_WIN_DIFFERENCE) / volatility - \
        ANTI_INFLATION
    if result == 1:
        return round(max(0.0, rating_adjustment))
    elif result == -1:
        return round(min(0.0, rating_adjustment))
    return round(rating_adjustment)
