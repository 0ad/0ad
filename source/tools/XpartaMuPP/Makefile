include_dir = /usr/lib/ejabberd/include/
ebin_dir = /usr/lib/ejabberd/ebin/

module = mod_ipstamp.beam
all : ${module}

%.beam : %.erl
	erlc -I ${include_dir} -pz ${ebin_dir} $<

install : 
	mv ${module} ${ebin_dir}

clean :
	rm ${module}

restart :
	rm -f /var/log/ejabberd/ejabberd.log
	/etc/init.d/ejabberd restart
