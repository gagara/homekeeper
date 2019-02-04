#!/bin/bash

source /etc/environment

query_stats() {
    response=$(curl -sS -w "http_code:%{http_code}" "$1" 2>&1)
    return_code=$?
    body=$(echo $response | sed -e 's/\(.*\)http_code\:\(.*\)/\1/')
    http_code=$(echo $response | sed -e 's/\(.*\)http_code\:\(.*\)/\2/')
    
    if [ $return_code -eq 0 -a $http_code -eq 200 ]; then
        response=$(echo $body | curl -sS -w "http_code:%{http_code}" -H "Content-type: application/json" -X POST "http://${FRONIUS_PIPELINE_HOST:-localhost}:${FRONIUS_PIPELINE_PORT:-80}" -d @- 2>&1)
        return_code=$?
        body=$(echo $response | sed -e 's/\(.*\)http_code\:\(.*\)/\1/')
        http_code=$(echo $response | sed -e 's/\(.*\)http_code\:\(.*\)/\2/')
        if [ $return_code -eq 0 -a $http_code -eq 200 ]; then
            return
        fi
    fi
    echo "[`date -u +%Y-%m-%dT%H:%M:%S,%3N`][WARN ][$return_code => $http_code] $body"
}

# delay required to allow inverter prepare archive stats for current timestamp
sleep 10

# current stats
if [ $((10#$(date +%M) % ${FRONIUS_CURR_RQ_PULL_INTERVAL:-1})) -eq 0 ]; then
    query_stats "http://${FRONIUS_HOST:-localhost}:${FRONIUS_PORT:-80}/solar_api/v1/GetInverterRealtimeData.cgi?Scope=Device&DeviceId=1&DataCollection=CommonInverterData" 
fi

# archive stats
if [ $((10#$(date +%M) % ${FRONIUS_ARCH_RQ_PULL_INTERVAL:-1})) -eq 0 ]; then
    query_stats "http://${FRONIUS_HOST:-localhost}:${FRONIUS_PORT:-80}/solar_api/v1/GetArchiveData.cgi?Scope=Device&DeviceClass=Inverter&DeviceId=1&StartDate=$(date -u --date=@$(($(date -u +%s) - $((${FRONIUS_ARCH_RQ_PULL_INTERVAL:-1} * 60)))) +%Y-%m-%dT%H:%M:%S)&EndDate=$(date -u +%Y-%m-%dT%H:%M:%S)&Channel=Current_DC_String_1&Channel=Current_DC_String_2&Channel=Voltage_DC_String_1&Channel=Voltage_DC_String_2&Channel=Temperature_Powerstage" 
fi
