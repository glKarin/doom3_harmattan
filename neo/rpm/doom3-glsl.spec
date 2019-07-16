Name:		doom3-glsl
Version:	1.3.1.1304harmattan1
Release:	1304
Summary:	DOOM III GLSL on wayland for Nemo.

Vendor: karin <beyondk2000@gmail.com>
Packager: karin <beyondk2000@gmail.com>
Group:		Amusements/Games
License:	GPLv3
URL:		https://github.com/glKarin/doom3glslwayland4nemo
Source0:	%{name}.tar.gz
BuildRoot:	%(mktemp -ud %{name})

BuildRequires:	pkgconfig(wayland-client)
BuildRequires:	pkgconfig(wayland-egl)
BuildRequires:	pkgconfig(egl)
BuildRequires:	pkgconfig(glesv2)
BuildRequires:	pkgconfig(alsa)
BuildRequires:	pkgconfig(openal)
Requires:	libhybris-libGLESv2
Requires:	libhybris-libEGL
Requires:	libhybris-libwayland-egl
Requires:	wayland
Requires:	OpenAL
Requires: alsa-lib

%description
DOOM III - glsl + touch on wayland for Nemo.
 Doom 3 is a survival horror first-person shooter video game developed by id Software and published by Activision. 
 A massive demonic invasion has overwhelmed the Union Aerospace Corporation's (UAC) Mars Research Facility, leaving only chaos and horror in its wake. As one of only a few survivors, you must fight your way to hell and back against a horde of evil monsters. The path is dark and dangerous, but you'll have an array of weapons--including a pistol, a chainsaw, grenades, and more--to use for protection.
 To run the 'base' game copy game data from original game to '/home/nemo/.doom3/base' folder.
 To run the 'Resurrection of Evil' game copy game data from original game to '/home/nemo/.doom3/d3xp' folder.
 User executale script is '/home/nemo/.doom3/DOOM_III.sh', if the script file is exists, it will execute user script, else execute global default command. See also in shell script: '/usr/lib/doom3-glsl/DOOM_III-glsl'.


%prep
%setup -q -n %{name}


%build
qmake
make


%install
rm -rf $RPM_BUILD_ROOT
# shell
install -D -m 755 debian/DOOM_III $RPM_BUILD_ROOT/usr/lib/doom3-glsl/DOOM_III-glsl
# desktop
install -D -m 644 debian/doom3.desktop $RPM_BUILD_ROOT/usr/share/applications/doom3-glsl.desktop
install -D -m 644 debian/doom3.png $RPM_BUILD_ROOT/usr/share/icons/hicolor/80x80/apps/doom3-glsl_80.png
# binary
install -D -m 755 doom3.arm $RPM_BUILD_ROOT/usr/bin/doom3.arm
# lib
install -D -m 755 libgame.so.1.0.0 $RPM_BUILD_ROOT/usr/lib/doom3-glsl/base/gamearm.so
install -D -m 755 libd3xp.so.1.0.0 $RPM_BUILD_ROOT/usr/lib/doom3-glsl/d3xp/gamearm.so
# resource
install -D -m 644 debian/cfg/DoomConfig.cfg $RPM_BUILD_ROOT/usr/lib/doom3-glsl/DoomConfig.cfg
mkdir -p $RPM_BUILD_ROOT/usr/lib/doom3-glsl/resc
install -D -m 644 resc/*.png $RPM_BUILD_ROOT/usr/lib/doom3-glsl/resc
mkdir -p $RPM_BUILD_ROOT/usr/lib/doom3-glsl/resc/glsl
install -D -m 644 resc/glsl/* $RPM_BUILD_ROOT/usr/lib/doom3-glsl/resc/glsl
# glsl
mkdir -p $RPM_BUILD_ROOT/usr/lib/doom3-glsl/gl2progs
install -D -m 644 gl2progs/* $RPM_BUILD_ROOT/usr/lib/doom3-glsl/gl2progs


%clean
make clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc
/usr/lib/doom3-glsl/DOOM_III-glsl
/usr/share/applications/doom3-glsl.desktop
/usr/share/icons/hicolor/80x80/apps/doom3-glsl_80.png
/usr/bin/doom3.arm
/usr/lib/doom3-glsl/base/gamesarm.so
/usr/lib/doom3-glsl/d3xp/gamesarm.so
/usr/lib/doom3-glsl/DoomConfig.cfg
/usr/lib/doom3-glsl/resc
/usr/lib/doom3-glsl/resc/glsl
/usr/lib/doom3-glsl/gl2progs



%changelog
* Fri Sep 28 2018 Karin Zhao <beyondk2000@gmail.com> - 1.3.1.1304harmattan1
  * OpenGL ES2.0 GLSL.
	* Wayland.
	* h_rNoLighting for disable lighting.


%post
INSTALL_DIR=/usr/lib/poom3-glsl
DIII_HOME=/home/nemo/.doom3
BASE=base
D3XP=d3xp

if [ "$1" = "1" ]; then
	if [ ! -d ${DIII_HOME} ]; then
		mkdir ${DIII_HOME}
		chown nemo:nemo ${DIII_HOME}
		echo "mkdir ${DIII_HOME}"
		if [ x$? != "x0" ]; then
			echo "[Error]: Create DOOM III home directory fail! Exiting..."
			exit 1;
		fi
	fi

	for game in ${BASE} ${D3XP}; do
#dir
		if [ ! -d ${DIII_HOME}/${game} ]; then
			mkdir ${DIII_HOME}/${game}
			chown nemo:nemo {DIII_HOME}/${game}
			echo "mkdir ${DIII_HOME}/${game}"
			if [ x$? != "x0" ]; then
				echo "[Error]: Create DOOM III ${game} directory fail! Exiting..."
				exit 2;
			fi
		fi
#lib
		if [ -e ${DIII_HOME}/${game}/DoomConfig.cfg ]; then
			rm -f ${DIII_HOME}/${game}/DoomConfig.cfg
			echo "rm -f ${DIII_HOME}/${game}/DoomConfig.cfg"
		fi
		cp -f ${INSTALL_DIR}/${game}/DoomConfig.cfg ${DIII_HOME}/${game}/
		chown nemo:nemo ${INSTALL_DIR}/${game}/DoomConfig.cfg
		echo "cp -f ${INSTALL_DIR}/${game}/DoomConfig.cfg ${DIII_HOME}/${game}/"

		if [ -e ${DIII_HOME}/${game}/gamearm.so ]; then
			rm -f ${DIII_HOME}/${game}/gamearm.so
			echo "rm -f ${DIII_HOME}/${game}/gamearm.so"
		fi
		ln -s ${INSTALL_DIR}/${game}/gamearm.so ${DIII_HOME}/${game}/
		echo "ln -s ${INSTALL_DIR}/${game}/gamearm.so ${DIII_HOME}/${game}/"
#glsl
		if [ ! -d ${DIII_HOME}/${game}/gl2progs ]; then
			mkdir ${DIII_HOME}/${game}/gl2progs
			chown nemo:nemo {DIII_HOME}/${game}/gl2progs
			echo "mkdir ${DIII_HOME}/${game}/gl2progs"
			if [ x$? != "x0" ]; then
				echo "[Error]: Create DOOM III ${game} gl2progs directory fail! Exiting..."
				exit 3;
			fi
		fi
		rm -f ${DIII_HOME}/${game}/gl2progs/*.vert
		echo "rm -f ${DIII_HOME}/${game}/gl2progs/*.vert"
		rm -f ${DIII_HOME}/${game}/gl2progs/*.frag
		echo "rm -f ${DIII_HOME}/${game}/gl2progs/*.frag"
		ln -s ${INSTALL_DIR}/${game}/gl2progs/* ${DIII_HOME}/${game}/gl2progs/
		echo "ln -s ${INSTALL_DIR}/${game}/gl2progs/* ${DIII_HOME}/${game}/gl2progs/"
	done

	echo "DOOM III configure done."
fi


%preun
DIII_HOME=/home/nemo/.doom3
BASE=base
D3XP=d3xp

if [ "$1" = "0" ]; then
	for game in ${BASE} ${D3XP}; do
		if [ -d ${DIII_HOME}/${game} ]; then
			Q2LIB=`ls ${DIII_HOME}/${game}/*.so 2> /dev/null || true`
			for libso in ${Q2LIB}; do
				echo "rm -f ${libso}"
				rm -f ${libso}
			done
			if [ -d ${DIII_HOME}/${game}/gl2progs ]; then
				rm -rf ${DIII_HOME}/${game}/gl2progs
				echo "rm -rf ${DIII_HOME}/${game}/gl2progs"
			fi
		fi
	done

	echo "DOOM III remove done."
fi
