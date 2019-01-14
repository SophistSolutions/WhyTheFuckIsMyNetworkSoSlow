
Why-The-Fuck-Is-My-Network-So-Slow Backend service app.

 *  To test:
    * Run the service (under the debugger if you wish)
    * curl  http://localhost:8080/ OR
    * curl  http://localhost:8080/FRED OR      (to see error handling)
    * curl -H "Content-Type: application/json" -X POST -d '{"AppState":"Start"}' http://localhost:8080/SetAppState
