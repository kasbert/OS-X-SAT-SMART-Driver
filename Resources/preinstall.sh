#!/bin/sh
kextunload -v -b org.dungeon.driver.SATSMARTDriver
kextunload -v -b fi.dungeon.driver.SATSMARTDriver
exit 0
