#!/bin/bash

# SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

function run_notify() {
    notify-send -a dde-file-manager "$@"
}

function run_pkexec() {
        xhost +SI:localuser:root
        echo "run in pkexec: $@"
        pkexec --disable-internal-agent "$@" `id -u`
        xhost -SI:localuser:root
        xhost
}

function run_app() {
        echo "param in run_app: $@"
        uid=$3
        echo "runner uid: $uid"
        export XDG_RUNTIME_DIR=/run/user/$uid
        export WAYLAND_DISPLAY=wayland-0
        export DISPLAY=:0
        export QT_WAYLAND_SHELL_INTEGRATION=kwayland-shell
        export XDG_SESSION_TYPE=wayland
        export QT_QPA_PLATFORM=
        export GDK_BACKEND=x11
        export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$uid/bus

        if [ "$1" == "-e" ];then
                deepin-editor $1
        elif [ "$1" == "-u" ];then
                dpkg -r "$2"
        fi
}

function run_package() {
        run_notify "打包程序" "开始打包软件 $1"
	rootDir="/tmp/imever/$1"
	[ -d $rootDir ] &&  rm -rf $rootDir
	[ -e "$rootDir.deb" ] && rm "$rootDir.deb"
	mkdir -p $rootDir
	mkdir "$rootDir/DEBIAN"
	for f in `dpkg --listfiles $1`;do
		# echo $f
		if [ "$f" == "/." ];then
			# echo "Got /. dir, skip"
			continue
		elif [ -d $f ];then 
			mkdir "$rootDir$f"
			continue
		elif [ -e $f ];then 
			cp -p $f "$rootDir$f"
			continue
		fi
	done
        for f in changelog conffiles config copyright list md5sums postinst postrm preinst prerm shlibs symbols templates triggers;do
		# echo "/var/lib/dpkg/info/$1.$f"
		if [ -e "/var/lib/dpkg/info/$1.$f" ];then
			cp -p "/var/lib/dpkg/info/$1.$f" "$rootDir/DEBIAN/$f"
		fi
	done
	dpkg --status $1 | grep -v 'Status: install ok installed' > "$rootDir/DEBIAN/control"
	dpkg-deb --build $rootDir "/tmp/imever/$1.deb" > /dev/null 2>&1
        run_notify "打包程序" "完成打包软件 $1"
        rm -rf "$rootDir"
        [ -e "/tmp/imever/$1.deb" ] && xdg-open "/tmp/imever"
}

# echo "return dde-file-manager in $XDG_SESSION_TYPE"
if [ "$XDG_SESSION_TYPE" == "x11" ];then
        if [ "$1" == "-e" ];then
                pkexec deepin-editor "$2"
        elif [ "$1" == "-u" ];then
                pkexec dpkg -r "$2"
	elif [ "$1" == "-p" ];then
		run_package "$2"
        fi
else
        echo "current file: $0"
        if [ x`id -u` != "x0" ];then
                run_pkexec $0 $@
                exit 0
        fi

        echo "run app: $@"
        run_app $@
fi
