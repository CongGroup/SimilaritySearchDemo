These old codes are not good, but still can run.
The recent opencv version dismiss the libopencv_nonfree.
You must add it by yourself if you use apt-get to install the opencv-dev
Steps below:

apt-get install libopencv-dev

sudo add-apt-repository --yes ppa:xqms/opencv-nonfree
sudo apt-get update 
sudo apt-get install libopencv-nonfree-dev

The dataset is located on /data/InformationDayDemoData/

including kmeans.vob data/ bow/

please copy the demo data to the above position.

Good Luck