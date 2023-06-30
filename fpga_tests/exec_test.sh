DIR1=results/$2/$3/$1
DIR2=results/$2/$3
DIR3=results/$2
DIR4=results
if [ ! -d "$DIR1" ]; then
  if [ ! -d "$DIR2" ]; then
    if [ ! -d "$DIR3" ]; then
      if [ ! -d "$DIR4" ]; then
            mkdir "$DIR4"
          fi
      mkdir "$DIR3"
    fi
    mkdir "$DIR2"
  fi
  mkdir "$DIR1"
  echo "$DIR1 created."
fi
cd client
if [ "$6" = "-debug" ]; then
  make clean
  make
fi
if [ "$5" = "-save" ]
  then sudo ./serial /dev/ttyUSB2 s1000 "$1" "$4" > ../"$DIR1"/"$1"_"$4"_res.txt
  else sudo ./serial /dev/ttyUSB2 s1000 "$1" "$4"
fi

