FROM ubuntu:focal as wine

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive
ENV LC_ALL C.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
ENV WINEDLLOVERRIDES "mscoree,mshtml="

ARG wine_ver='winehq-staging'
ARG wineticks_ver='master'

RUN dpkg --add-architecture i386 && \
    apt-get -y update && apt-get -y install wget tar gnupg2 && \
    wget -O - https://dl.winehq.org/wine-builds/winehq.key | apt-key add -  && \
    echo 'deb https://dl.winehq.org/wine-builds/ubuntu/ focal main' |tee /etc/apt/sources.list.d/winehq.list && \
    apt-get -y update && apt-get -y full-upgrade && \
    apt-get -y install $wine_ver xvfb cabextract winbind && \
    wget -O /usr/bin/winetricks https://raw.githubusercontent.com/Winetricks/winetricks/$wineticks_ver/src/winetricks && chmod +x /usr/bin/winetricks && \
    apt-get clean && rm -rf /var/lib/apt/lists/*


FROM wine

ENV WINEARCH win64

ADD --chown=root:root wine_prefix/ /root/.wine

# update wineenv
RUN xvfb-run wineboot
