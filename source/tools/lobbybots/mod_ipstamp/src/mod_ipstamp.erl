%% Copyright (C) 2018 Wildfire Games.
%% This file is part of 0 A.D.
%%
%% 0 A.D. is free software: you can redistribute it and/or modify
%% it under the terms of the GNU General Public License as published by
%% the Free Software Foundation, either version 2 of the License, or
%% (at your option) any later version.
%%
%% 0 A.D. is distributed in the hope that it will be useful,
%% but WITHOUT ANY WARRANTY; without even the implied warranty of
%% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%% GNU General Public License for more details.
%%
%% You should have received a copy of the GNU General Public License
%% along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.

-module(mod_ipstamp).

-behaviour(gen_mod).

-include("ejabberd.hrl").
-include("logger.hrl").
-include("xmpp.hrl").

-export([start/2,
         stop/1,
         depends/2,
         mod_opt_type/1,
         reload/3,
         on_filter_packet/1]).

start(_Host, _Opts) ->
    ejabberd_hooks:add(filter_packet, global, ?MODULE, on_filter_packet, 50).

stop(_Host) ->
    ejabberd_hooks:delete(filter_packet, global, ?MODULE, on_filter_packet, 50).

depends(_Host, _Opts) -> [].

mod_opt_type(_) -> [].

reload(_Host, _NewOpts, _OldOpts) -> ok.

-spec on_filter_packet(Input :: iq()) -> iq() | drop.
on_filter_packet(#iq{type = set, to = To, sub_els = [SubEl]} = Input) ->
    % We only want to do something for the bots
    case acl:match_rule(global, ipbots, To) of
      allow ->
        NS = xmpp:get_ns(SubEl),
        if NS ==  <<"jabber:iq:gamelist">> ->
          SCommand = fxml:get_path_s(SubEl, [{elem, <<"command">>}, cdata]),
          if SCommand == <<"register">> ->
            % Get the sender's IP.
            Ip = xmpp:get_meta(Input, ip),
            SIp = inet_parse:ntoa(Ip),
            ?INFO_MSG(string:concat("Inserting IP into game registration "
                                    "stanza: ", SIp), []),
            Game = fxml:get_subtag(SubEl, <<"game">>),
            GameWithIp = fxml:replace_tag_attr(<<"ip">>, SIp, Game),
            SubEl2 = fxml:replace_subtag(GameWithIp, SubEl),
            xmpp:set_els(Input, [SubEl2]);
          true ->
            Input
          end;
        true ->
          Input
        end;
      _ -> Input
    end;

on_filter_packet(Input) ->
  Input.
