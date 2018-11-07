# 0 A.D. / Pyrogenesis Multiplayer Lobby Setup

This README explains how to setup a custom Pyrogenesis Multiplayer Lobby server that can be used with the Pyrogenesis game.

## Service description
The Pyrogenesis Multiplayer Lobby consists of three components:

* **XMPP server: ejabberd**:
    The XMPP server provides the platform where users can register accounts, chat in a public room, and can interact with lobby bots.
    Currently, ejabberd is the only XMPP server software supported (by the ipstamp module for XpartaMuPP).

* **Gamelist bot: XpartaMuPP**:
    This bot allows players to host and join online multiplayer matches.
    It utilizes the ejabberd ipstamp module to inform players of IP addresses of hosting players.

* **Rating bot: EcheLOn**:
    This bot allows players to gain a rating that reflects their skill based on online multiplayer matches.
    It is by no means necessary for the operation of a lobby in terms of match-making and chatting.

## Service choices
Before installing the service, you have to make some decisions:

#### Choice: Domain Name
Decide on a domain name where the service will be provided.
This document will use `lobby.wildfiregames.com` as an example.
If you intend to use the server only for local testing, you may choose `localhost`.

#### Choice: Rating service
Decide whether or not you want to employ the rating service.
If you decide to not provide the rating service, you may skip the instructions for the rating bot in this document.

#### Choice: Pyrogenesis version compatibility
Decide whether you want to support serving multiple Pyrogenesis versions.

Serving multiple versions of Pyrogenesis allows for seamless version upgrading on the backend and
allows players that don't have the most recent version of Pyrogenesis yet to continue to play until
the new release is available for their platform (applies mostly to linux distributions).

If you decide to do so, you should use a naming pattern that includes the targetted Pyrogenesis version.
For example to provide a Multiplayer Lobby for Pyrogenesis Alpha 23 "Ken Wood",
name the lobby room `arena23` instead of `arena` and use `xpartamupp23` and `echelon23` as lobby bot names.
Then when a version 24 of Pyrogenesis is employed, you can easily add `arena24`, `xpartamupp24` and `echelon24`.
If you only want to use the service for local testing, you can stick to a single room and a single gamelist and rating bot.

## 1. Install dependencies

This section explains how to install the required software on a Debian-based linux distribution.
For other operating systems, use the according package manager or consult the official documentation of the software.

### 1.1 Install ejabberd

The version requirement for ejabberd is 17.03 or later (due to the ipstamp module format).

* Install `ejabberd` using the following command. Alternatively see <https://docs.ejabberd.im/admin/installation/>.

    ```
    $ apt-get install ejabberd
    ```

* Confirm that the ejabberd version you installed is the one mentioned above or later:

    ```
    $ ejabberdctl status
    ```

* Configure ejabberd by setting the domain name of your choice and add an `admin` user.:

    ```
    $ dpkg-reconfigure ejabberd
    ````

You should now be able to connect to this XMPP server using any XMPP client.

### 1.2 Install python3 and SleekXmpp

* The lobby bots are programmed in python3 and use SleekXMPP to connect to the lobby. Install these dependencies using:

    ```
    $ apt-get install python3 python3-sleekxmpp
    ```

* Confirm that the SleekXmpp version is 1.3.1 or later:

    ```
    pip3 show sleekxmpp
    ```

* If you would like to run the rating bot, you will need to install SQLAlchemy for python3:

    ```
    $ apt-get install python3-sqlalchemy
    ```

### 1.3 Install ejabberd ipstamp module

The ejabberd ipstamp module has the purpose of inserting the IP address of the hosting players into the gamelist packet.
That enables players to connect to each others games.

* Adjust `/etc/ejabberd/ejabberdctl.cfg` and set `CONTRIB_MODULES_PATH` to the directory where you want to store `mod_ipstamp`:

    ```
    CONTRIB_MODULES_PATH=/opt/ejabberd-modules
    ```

* Ensure the target directory is readable by ejabberd.
* Copy the `mod_ipstamp` directory from `XpartaMuPP/` to `CONTRIB_MODULES_PATH/sources/`.
* Check that the module is available and compatible with your ejabberd:

    ```
    $ ejabberdctl modules_available
    $ ejabberdctl module_check mod_ipstamp
    ```

* Install `mod_ipstamp`:

    ```
    $ ejabberdctl module_install mod_ipstamp
    ```

## 2. Configure ejabberd mod_ipstamp

The ejabberd configuration in the remainder of this document is performed by editing `/etc/ejabberd/ejabberd.yml`.
The directory containing this README includes a preconfigured `ejabberd_example.yml` that only needs few setting changes to work with your setup.
For a full documentation of the ejabberd configuration, see <https://docs.ejabberd.im/admin/configuration/>.
If something goes wrong with ejabberd, check `/var/log/ejabberd/ejabberd.log`

* Add `mod_ipstamp` to the modules ejabberd should load:

    ```
    modules:
      mod_ipstamp: {}
    ```

* Reload the ejabberd config.
  This should be done every few steps, so that configuration errors can be identified as soon as possible.

    ```
    $ ejabberdctl reload_config
    ```

## 3. Configure ejabberd connectivity

The settings in this section ensure that connections can be built where intended, and only where intended.

### 3.1 Disable IPv6
* Since the enet library which Pyrogenesis uses for multiplayer mode does not support IPv6, ejabberd must be configured to not use IPv6:

    ```
    listen:
        ip: "0.0.0.0"
    ```

### 3.2 Enable STUN
* ejabberd and Pyrogenesis support the STUN protocol. This allows players to connect to each others games even if the host did not configure
the router and forward the UDP port. Enabling STUN is optional but recommended.

    ```
    listen:
        -
        port: 3478
        transport: udp
        module: ejabberd_stun
    ```

### 3.3 Enable keep-alive

* This helps with users becoming disconnected:

    ```
    modules:
      mod_ping:
        send_pings: true
    ```

### 3.3 Disable unused services

* Disable the currently unused server-to-server communication:

    ```
    listen:
        ## -
        ##   port: 5269
        ##   ip: "::"
        ##   module: ejabberd_s2s_in
    ```

* Protect the administrative webinterface at <https://localhost:5280/admin> from external access by disabling or restriction to `localhost`:

    ```
    listen:
      -
        port: 5280
        ip: "127.0.0.1"
    ```

* Disable some unused modules:

    ```
    modules:
      ## mod_echo: {}
      ## mod_irc: {}
      ## mod_shared_roster: {}
      ## mod_vcard: {}
      ## mod_vcard_xupdate: {}
    ```

### 3.4 Setup TLS encryption

Depending on whether you use the server for a player audience or only for local testing,
you may have to either obtain and install a certificate with ejabberd or disable TLS encryption.

#### Choice A: No encryption
* If you intend to use the server solely for local testing, you may disable TLS encryption in the ejabberd config:

    ```
    listen:
        starttls_required: false
    ```

#### Choice B: Self-signed certificate

If you want to use the server for local testing only, you may use a self-signed certificate to test encryption.
Notice the lobby bots currently reject self-signed certificates.

* Enable TLS over the default port:
    ```
    listen:
        starttls: true
    ```

* Create the key file for certificate:

    ```
    openssl genrsa -out key.pem 2048
    ```
* Create the certificate file.  “common name” should match the domainname.

    ```
    openssl req -new -key key.pem -out request.pem
    ```

* Sign the certificate:

    ```
    openssl x509 -req -days 900 -in request.pem -signkey key.pem -out certificate.pem
    ```

* Store it as the ejabberd certificate:

    ```
    $ cat key.pem request.pem > /etc/ejabberd/ejabberd.pem
    ```

#### Choice C: Let's Encrypt certificate
To secure user authentication and communication with modern encryption and to comply with privacy laws,
ejabberd should be configured to use TLS with a proper, trusted certificate.

* A free, valid, and trusted TLS certificate may be obtained from some certificate authorites, such as Let's Encrypt:
<https://letsencrypt.org/getting-started/>
<https://blog.process-one.net/securing-ejabberd-with-tls-encryption/>

* Enable TLS over the default port:
    ```
    listen:
        starttls: true
    ```

* Setup the contact address if Let's Encrypt found an authentication issue:

    ```
    acme:
      contact: "mailto:admin@example.com"
    ```

* Ensure old, vulnerable SSL/TLS protocols are disabled:

    ```
    define_macro:
      'TLS_OPTIONS':
        - "no_sslv2"
        - "no_sslv3"
        - "no_tlsv1"
	```

## 3. Configure ejabberd use policy

The settings in this section grant or restrict user access rights.

* Prevent the rooms from being destroyed if the last client leaves it:

    ```
    access_rules:
      muc_admin:
        - allow: admin
    modules:
      mod_muc:
        access_persistent: muc_admin
        default_room_options:
          persistent: true
    ```

* Allow users to create accounts using the game via in-band registration.
    ```
    access_rules:
      register:
        - all: allow
    ```

### Optional use policies

* (Optional) It is recommended to restrict usernames to alphanumeric characters (so that playernames are easily typeable for every participant).
  The username may be restricted in length (because very long usernames are uncomfortably time-consuming to read and may not fit into the playername fields).
  Notice the username regex below is also used by the 0 A.D. client to indicate invalid names to the user.
    ```
    acl:
      validname:
        user_regexp: "^[0-9A-Za-z._-]{1,20}$"

    access_rules:
      register:
        - allow: validname

    modules:
      mod_register:
        access: register
    ```

* (Optional) Prevent users from creating new rooms:

    ```
    modules:
      mod_muc:
        access_create: muc_admin
    ```

* (Optional) Increase the maximum number of users from the default 200:

    ```
      mod_muc:
        max_users: 5000
        default_room_options:
            max_users: 1000
    ```

* (Optional) Prevent users from sending too large stanzas.
  Notice the bots can send large stanzas as well, so don't restrict it too much.

	```
    max_stanza_size: 1048576
	```


* (Optional) Prevent users from changing the room topic:

    ```
      mod_muc:
        default_room_options:
          allow_change_subj: false
    ```

* (Optional) Prevent malicious users from registering new accounts quickly if they were banned.
  Notice this also prevents players using the same internet router from registering for that time if they want to play together.

    ```
    registration_timeout: 3600
    ```

* (Optional) Enable room chatlogging.
  Make sure to mention this collection and the purposes in the Terms and Conditions to comply with personal data laws.
  Ensure that ejabberd has write access to the given folder.
  Notice that `ejabberd.service` by default prevents write access to some directories (PrivateTmp, ProtectHome, ProtectSystem).

    ```
      modules:
        mod_muc_log:
          outdir: "/lobby/logs"
          file_format: plaintext
          timezone: universal
        mod_muc:
          default_room_options:
            logging: true
    ```

* (Optional) Grant specific moderators administrator rights to see the IP address of a user:
    See also `https://xmpp.org/extensions/xep-0133.html#get-user-stats`.

    ```
    acl:
      admin:
        user:
          - "username@lobby.wildfiregames.com"
    ```

* (Optional) Grant specific moderators to :
    See also `https://xmpp.org/extensions/xep-0133.html#get-user-stats`.

    ```
    modules:
      mod_muc:
        access_admin: muc_admin
    ```

* (Optional) Ban specific IP addresses or subnet masks for persons that create new accounts after having been banned from the room:

    ```
    acl:
      blocked:
        ip:
          - "12.34.56.78"
          - "12.34.56.0/8"
          - "12.34.0.0/16"
    ...
    access_rules:
      c2s:
        - deny: blocked
        - allow
      register:
        - deny: blocked
        - allow
    ```

## 4. Setup lobby bots

### 4.1 Register lobby bot accounts

* Check list of registered users:

    ```
    $ ejabberdctl registered_users lobby.wildfiregames.com
    ```

* Register the accounts of the lobby bots.
  The rating account is only needed if you decided to enable the rating service.

    ```
    $ ejabberdctl register echelon23 lobby.wildfiregames.com secure_password
    $ ejabberdctl register xpartamupp23 lobby.wildfiregames.com secure_password
    ```

### 4.2 Authorize lobby bots to see real JIDs

* The bots need to be able to see real JIDs of users.
  So either the room must be configured as non-anonymous, i.e. real JIDs are visible to all users of the room,
  or the bots need to receive muc administrator rights.

#### Choice A: Non-anonymous room
* (Recommended) This method has the advantage that bots do not gain administrative access that they don't use.
  The only possible downside is that room users may not hide their username behind arbitrary nicknames anymore.

    ```
    modules:
      mod_muc:
        default_room_options:
          anonymous: false

#### Choice B: Non-anonymous room
* If you for any reason wish to configure the room as semi-anonymous (only muc administrators can see real JIDs),
  then the bots need to be authorized as muc administrators:

    ```
    access_rules:
      muc_admin:
        - allow: bots

    modules:
      mod_muc:
        access_admin: muc_admin
	```

### 4.3 Authorize lobby bots with ejabberd

* The bots need an ACL to be able to get the IPs of users hosting a match (which is what `mod_ipstamp` does).

    ```
        acl:
          ## Don't use a regex, to prevent others from obtaining permissions after registering such an account.
          bots:
            - user: "xpartamupp23@lobby.wildfiregames.com"
            - user: "echelon23@lobby.wildfiregames.com"
    ```

* Add an access rule for `ipbots` and a rule allowing bots to create PubSub nodes:

    ```
    access_rules:
      ## Expected by the ipstamp module for XpartaMuPP
      ipbots:
        - allow: bots

      pubsub_createnode:
        - allow: bots
    ```

* Due to the amount of traffic the bot may process, give the group containing bots either unlimited
  or a very high traffic shaper:

    ```
    shaper_rules:
      c2s_shaper:
      - none: admin, bots
      - normal
    ```

* Finally reload ejabberd's configuration:

    ```
    $ ejabberdctl reload_config
    ```

### 4.4 Running XpartaMuPP - XMPP Multiplayer Game Manager

* Execute the following command to run the gamelist bot:

    ```
    $ python3 XpartaMuPP.py --domain lobby.wildfiregames.com --login xpartamupp23 --password XXXXXX --nickname GamelistBot --room arena --elo echelon23
    ```

If you want to run XpartaMuPP without a rating bot, the `--elo` argument should be omitted.
Pass `--disable-tls` if you did not setup valid TLS encryption on the server.
Run `python3 XpartaMuPP.py --help` for the full list of options

* If the connection and authentication succeeded, you should see the following messages in the console:

    ```
    INFO     JID set to: xpartamupp23@lobby.wildfiregames.com/CC
    INFO     XpartaMuPP started
    ```

### 4.5 Running EcheLOn - XMPP Multiplayer Rating Manager

This bot can be thought of as a module of XpartaMuPP in that IQs stanzas sent to XpartaMuPP are
forwarded onto EcheLOn if its corresponding EcheLOn is online and ignored otherwise.
EcheLOn handles all aspects of operation related to ELO, the chess rating system invented by Arpad Elo.
Players gain a rating after a rated 1v1 match.
The score difference after a completed match is relative to the rating difference of the players.

* (Optional) Some constants of the algorithm may be edited by experienced administrators at the head of `ELO.py`:

	```
	# Difference between two ratings such that it is
	# regarded as a "sure win" for the higher player.
	# No points are gained or lost for such a game.
	elo_sure_win_difference = 600.0

	# Lower ratings "move faster" and change more
	# dramatically than higher ones. Anything rating above
	# this value moves at the same rate as this value.
	elo_k_factor_constant_rating = 2200.0
	```

* To initialize the `lobby_rankings.sqlite3` database, execute the following command:

    ```
    $ python3 LobbyRanking.py
    ```

* Execute the following command to run the rating bot:

    ```
    $ python3 EcheLOn.py --domain lobby.wildfiregames.com --login echelon23 --password XXXXXX --nickname RatingBot --room arena23
    ```

Run `python3 EcheLOn.py --help` for the full list of options

## 5. Configure Pyrogenesis for the new Multiplayer Lobby

The Pyrogenesis client is now going to be configured to become able to connect to the new Multiplayer Lobby.

The Pyrogenesis documentation of configuration files can be found at <https://trac.wildfiregames.com/wiki/Manual_Settings>.
Available Pyrogenesis configuration settings are specified in `default.cfg`, see <https://code.wildfiregames.com/source/0ad/browse/ps/trunk/binaries/data/config/default.cfg>.

### 5.1 Local Configuration

 * Visit <https://trac.wildfiregames.com/wiki/GameDataPaths> to identify the local user's Pyrogenesis configuration path depending on the operating system.

 * Create or open `local.cfg` in the configuration path.

 * Add the following settings that determine the lobby server connection:

    ```
    lobby.room = "arena23"                          ; Default MUC room to join
    lobby.server = "lobby.wildfiregames.com"        ; Address of lobby server
    lobby.stun.server = "lobby.wildfiregames.com"   ; Address of the STUN server.
    lobby.require_tls = true                        ; Whether to reject connecting to the lobby if TLS encryption is unavailable.
    lobby.verify_certificate = true                 ; Whether to reject connecting to the lobby if the TLS certificate is invalid.
    lobby.xpartamupp = "xpartamupp23"               ; Name of the server-side XMPP-account that manage games
    lobby.echelon = "echelon23"                     ; Name of the server-side XMPP-account that manages ratings
    ```

  If you disabled TLS encryption, set `require_tls` to `false`.
  If you employed a self-signed certificate, set `verify_certificate` to `false`.

### 5.2 Test the Multiplayer Lobby

You should now be able to join the new multiplayer lobby with the Pyrogenesis client and play multiplayer matches.

* To confirm that the match hosting works as intended, create two user accounts, host a game with one, join the game with the other account.

* To confirm that the rating service works as intended, resign a rated 1v1 match with two accounts.

### 5.3 Terms and Conditions

Players joining public servers are subject to Terms and Conditions of the service provider and subject to privacy laws such as GDPR.
If you intend to use the server only for local testing, you may skip this step.

* The following files should be created by the service provider:

	`Terms_of_Service.txt` to explain the service and the contract.
	`Terms_of_Use.txt` to explain what the user should and should not do.
	`Privacy_Policy.txt` to explain how personal data is handled.

* To use Wildfire Games Terms as a template, obtain our Terms from a copy of the game or from or from
<https://trac.wildfiregames.com/browser/ps/trunk/binaries/data/mods/public/gui/prelobby/common/terms/>

* Replace all occurrences of `Wildfire Games` in the files with the one providing the new server.

* Update the `Terms_of_Use.txt` depending on which behavior you would like to (not) see on your service.

* Update the `Privacy_Policy.txt` depending on the user data processing in relation to the usage policies.
Make sure to not violate privacy laws such as GDPR or COPPA while doing so.

* The retention times of ejabberd logs are relevant to GDPR.
Visit <https://www.ejabberd.im/Rotating%20logs%20with%20ejabberd/index.html> for details.

* The terms should be published online, so users can save and print them.
  Add to your `local.cfg`:

	```
	lobby.terms_url = "https://lobby.wildfiregames.com/terms/"; Allows the user to save the text and print the terms
	```

### 5.4 Distribute the configuration

To make this a public server, distribute your `local.cfg`, `Terms_of_Service.txt`, `Terms_of_Use.txt`, `Privacy_Policy.txt`.

It may be advisable to create a mod with a modified `default.cfg` and the new terms documents,
see <https://trac.wildfiregames.com/wiki/Modding_Guide>.

Congratulations, you are now running a custom Pyrogenesis Multiplayer Lobby!
