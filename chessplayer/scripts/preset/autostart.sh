#!/bin/bash

############# - step - ####################
####### cd path/to/scripts/preset/ ########
####### sudo ./autostart.sh ############### 
####### sudo reboot #######################

## disable old one ## 
systemctl disable robot >/dev/null 2>&1

program_dir=$(dirname $(dirname $(pwd)))

## set robot scprit ##
echo "#!/bin/bash
### BEGIN INIT INFO
#
# Provides:  location_server
# Required-Start: $local_fs  $remote_fs
# Required-Stop: $local_fs  $remote_fs
# Default-Start:  2 3 4 5
# Default-Stop:  0 1 6
# Short-Description: initscript
# Description:  This file should be used to construct scripts to be placed in /etc/init.d.
#
### END INIT INFO

cd $program_dir
echo '==========='\$(date '+%Y-%m-%d %H:%M:%S')'============' >log.robot
echo \"cd $program_dir\" >>log.robot

su tiger
echo 'su tiger' >>log.robot

source scripts/set_env.sh
echo 'source scripts/set_env.sh' >>log.robot

./scripts/start_global.sh
echo './scripts/start_global.sh' >>log.robot

exit 0 "> /etc/init.d/robot

chmod +x /etc/init.d/robot

## enable new one ##
systemctl enable robot
