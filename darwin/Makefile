X11-ENV := -e DISPLAY=docker.for.mac.host.internal:0 -v /tmp/.X11-unix:/tmp/.X11-unix

IMAGE_NAME := spacezero

run:
	docker run --rm ${X11-ENV} -w /src -v ${PWD}/../:/src ${IMAGE_NAME} make run

build:
	docker run --rm ${X11-ENV} -w /src -v ${PWD}/../:/src ${IMAGE_NAME} make build

clean:
	docker run --rm ${X11-ENV} -w /src -v ${PWD}/../:/src ${IMAGE_NAME} make clean

docker-build:
	cd ../.devcontainer && docker build -t ${IMAGE_NAME} .

docker-bash:
	docker run -it --rm ${X11-ENV} -w /src -v ${PWD}/../:/src ${IMAGE_NAME}

x11-setup:
	xhost +localhost

x11-test:
	docker run --rm ${X11-ENV} sshipway/xclock

x11-opengl:
	docker run -it --rm ${X11-ENV} -w /src -v ${PWD}/../:/src utensils/opengl:latest /src/bin/hello-gl