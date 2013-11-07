# Rating that new players should be inserted into the
# database with, before they've played any games.
default_rating = 1200

# Required minimum number of games to get on the
# leaderboard.
leaderboard_minimum_games = 10

# Required minimum number of games per month to
# qualify as an active player.
leaderboard_active_games = 5

# Difference between two ratings such that it is
# regarded as a "sure win" for the higher player.
# No points are gained or lost for such a game.
elo_sure_win_difference = 600

# Lower ratings "move faster" and change more
# dramatically than higher ones. Anything rating above
# this value moves at the same rate as this value.
elo_k_factor_constant_rating = 2200

