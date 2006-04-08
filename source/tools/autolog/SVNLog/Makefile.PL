use inc::Module::Install;

name('SVNLog');
abstract('Catalyst Application');
author('Catalyst developer');
version_from('lib/SVNLog.pm');
license('perl');

include('ExtUtils::AutoInstall');

requires( Catalyst => '5.60' );

catalyst_files();

install_script( glob('script/*.pl') );
auto_install();
&WriteAll;
