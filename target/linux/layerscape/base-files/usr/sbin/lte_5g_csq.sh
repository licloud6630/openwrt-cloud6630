#!/bin/sh

# 检测间隔（秒）
INTERVAL=5
USE_QMI=0

echo "LTE信号质量实时监测（每${INTERVAL}秒更新一次）"
echo "----------------------------------------"

nocsq_led() {
    echo 0 > /sys/class/leds/lte_0/brightness
    echo 0 > /sys/class/leds/lte_1/brightness
    echo 0 > /sys/class/leds/lte_2/brightness
}

highcsq_led() {
    echo 1 > /sys/class/leds/lte_0/brightness
    echo 1 > /sys/class/leds/lte_1/brightness
    echo 1 > /sys/class/leds/lte_2/brightness
}

midcsq_led() {
    echo 1 > /sys/class/leds/lte_0/brightness
    echo 1 > /sys/class/leds/lte_1/brightness
    echo 0 > /sys/class/leds/lte_2/brightness 
}

littlecsq_led() {
    echo 1 > /sys/class/leds/lte_0/brightness
    echo 0 > /sys/class/leds/lte_1/brightness
    echo 0 > /sys/class/leds/lte_2/brightness   
}

while true; do
    quectel_ec20=$(lsusb | grep -i "2c7c:0125")
    fibocom_nl668=$(lsusb | grep -i "1508:1001")
    quectel_rm500u=$(lsusb | grep -i "2c7c:0900")
    fibocom_fm150=$(lsusb | grep -i "2cb7:0104")

    sleep 3
    if [ -z "$quectel_ec20" ] && [ -z "$fibocom_nl668" ] && [ -z "$quectel_rm500u" ] && [ -z "$fibocom_fm150" ]; then
        continue
    else
        if [ -z "$quectel_rm500u" ]; then # no rm500u,use qmi
            USE_QMI=1
            echo "Find Quectel EC20 Or NL668 Or FM150/160,will use quectel qmi..."
        fi
        break
    fi
done

WAN2IP_OLD=$(ip -4 addr show dev wwan0 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d '/' -f 1)
while true; do
    # 1. 查询SIM卡在位;此功能
    CPIN=$(sms_tool -d /dev/ttyUSB2 at at+cpin? | grep READY)
    if [ -z "$CPIN" ]; then
        nocsq_led
        sleep 1
    else
        # 2. 查询信号强度（CSQ）
        CSQ=$(sms_tool -d /dev/ttyUSB2 at at+csq | grep CSQ | awk -F '[,:]' '{print $2}' | grep -oE '[0-9]+\.?[0-9]*')
        # 解析信号强度描述
        if [ -z "$CSQ" ]; then
            nocsq_led
        elif [ $CSQ -ge 25 ]; then
            highcsq_led
        elif [ $CSQ -ge 20 ]; then
            midcsq_led
        elif [ $CSQ -ge 15 ]; then
            littlecsq_led
        elif [ $CSQ -ge 10 ]; then
            littlecsq_led
        else
            nocsq_led
        fi
    fi

    # 3. Luci上quectel qmi协议如果拔插卡就不会显示IP了，这里加一个检测
    if [ $USE_QMI -ge 1 ]; then
        WAN2IP_NOW=$(ip -4 addr show dev wwan0 2>/dev/null | grep 'inet ' | awk '{print $2}' | cut -d '/' -f 1)
        if [ "$WAN2IP_NOW" != "$WAN2IP_OLD" ] && [ "$WAN2IP_NOW" ]; then
            echo "IP Change..."
            #表示IP发生过变化
            ifdown wan2&& sleep 1
            ifup wan2
            WAN2IP_OLD=$WAN2IP_NOW
        fi
    fi
    
    sleep $INTERVAL
done