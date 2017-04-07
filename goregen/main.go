package main

import (
	"time"

	"flag"
	"fmt"
	"gobot.io/x/gobot"
	"gobot.io/x/gobot/drivers/gpio"
	"gobot.io/x/gobot/platforms/firmata"
	"log"
)

var (
	device = flag.String("dev", "/dev/ttyUSB0", "path to serial port")
)

func init() {
	flag.Parse()
}

func main() {
	firmataAdaptor := firmata.NewAdaptor(*device)
	devices := []gobot.Device{
		gpio.NewLedDriver(firmataAdaptor, "13"),
	}

	comm := make(chan string, 10)
	robot := gobot.NewRobot("bot",
		[]gobot.Connection{firmataAdaptor},
		devices,
	)
	robot.AutoRun = false

	robot.Work = func() {
		comm <- "started working"
		gobot.Every(time.Second, func() {
			for _, v := range devices {
				if led, ok := v.(*gpio.LedDriver); ok {
					if led.State() {
						comm <- "switched-off"
						led.Off()
					} else {
						comm <- "switched-on"
						led.On()
					}
				}
			}
		})
	}

	go func() {
		for msg := range comm {
			fmt.Println(msg)
		}
	}()

	err := robot.Start()
	if err != nil {
		log.Fatal(err)
	}

	<-time.After(5 * time.Second)
	robot.Stop()
	fmt.Println("bye now am tired")
}
