#include "service.h"

// Msdn documentation: 
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms685141%28v=vs.85%29.aspx
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms687416%28v=vs.85%29.aspx

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

/*
 int ServiceInstall()
 https://msdn.microsoft.com/en-us/library/windows/desktop/ms683500%28v=vs.85%29.aspx

*/
int ServiceInstall( ) {

	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	char binaryPath[MAX_PATH] = {'\0'};

	// Get the path of the current binary
	if (!GetModuleFileName(NULL, binaryPath, MAX_PATH)) {
		printf("[-] Error :: ServiceInstall!GetModuleFileName() failed :: exit_code %d\n",GetLastError());
		return -1;
	}

	// Get a handle to the SCM database.
	schSCManager = OpenSCManager(NULL,						// Local computer.
								 NULL,						// ServicesActive database.
								 SC_MANAGER_ALL_ACCESS) ;	// full access rights.

	if (schSCManager == NULL) {
		printf("[-] Error :: ServiceInstall!OpenSCManager() failed :: exit_code = %d\n",GetLastError());
		return -1;
	}


	// Create the service.
	schService = CreateService( 
		schSCManager,				// SCM database 
		SVCNAME,					// name of service 
		SVCDISPLAY,					// service name to display 
		SERVICE_ALL_ACCESS,			// desired access 
		SERVICE_WIN32_OWN_PROCESS,	// service type 
		SERVICE_DEMAND_START,		// start type
		SERVICE_ERROR_NORMAL,		// error control type 
		binaryPath,					// path to service's binary 
		NULL,						// no load ordering group 
		NULL,						// no tag identifier 
		NULL,						// no dependencies 
		NULL,						// LocalSystem account 
		NULL);						// no password 


	if (schService == NULL) {
		printf("[-] Error :: ServiceInstall!CreateService() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schSCManager);
		return -1;
	}
	else {
		printf("[+] Debug :: Service installed successfully!\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 0;
}


/*
	ServiceRemove()
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms682571%28v=vs.85%29.aspx
*/
int ServiceRemove( ) {

	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_STATUS_PROCESS ssStatus;
	DWORD dwBytesNeeded;

	// Get a handle to the SCM database.
	schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (schSCManager == NULL){
        printf("[-] Error :: ServiceRemove!OpenSCManager() failed (%d)\n", GetLastError());
        return -1;
    }

	// Get a handle to the service.
	schService = OpenService(schSCManager, SVCNAME, DELETE|SERVICE_QUERY_STATUS);
	if (schService == NULL) {
		printf("[-] Error :: ServiceRemove!OpenService() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schSCManager);
		return -1;
	}
	
	// Check the status in case the service is started. If it's the case, then stop it first.
	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
		printf("[-] Error :: ServiceRemove!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return -1;
	}

	if (ssStatus.dwCurrentState == SERVICE_RUNNING || ssStatus.dwCurrentState == SERVICE_START_PENDING ) {
		printf("[i] Debug :: ServiceRemove :: Stopping the service...\n");
		ServiceStop( );
	}

	// Delete the service
	if (!DeleteService(schService)) {
		printf("[-] Error :: ServiceRemove!DeleteService() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return -1;
	}

	printf("[+] Debug :: Service deleted sucessfully !\n");


	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return 0;
}


/*
	ReportSvcStatus()
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms687414%28v=vs.85%29.aspx
*/
VOID ReportSvcStatus(DWORD dwCurrentState,
					 DWORD dwWin32ExitCode,
					 DWORD dwWaitHint) {

	static DWORD dwCheckPoint = 1;

	// Fill the service status structure

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (gSvcStatus.dwCurrentState == SERVICE_START_PENDING) {
		gSvcStatus.dwControlsAccepted = 0;
	}
	else
		gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
		gSvcStatus.dwCheckPoint = 0;
	else
		gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);

	return;

}


/*
	ServiceCtrlHandler()
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms687413%28v=vs.85%29.aspx
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms685149%28v=vs.85%29.aspx
*/
void WINAPI ServiceCtrlHandler( DWORD dwCtrl ) {

	switch (dwCtrl) {

		case SERVICE_CONTROL_STOP:
			ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

			 // Signal the service to stop.
			 SetEvent(ghSvcStopEvent);
			 ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);

			return;
		case SERVICE_CONTROL_INTERROGATE:
			break;

		default:
			break;
	}

	return;
}



/*
 LaunchServiceAction()
*/
void LaunchServiceAction( ) {

	// Perform service work.

	return;
}

/*
	ServiceInit()
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms687414%28v=vs.85%29.aspx
*/
void ServiceInit( ) {

	// TODO :: Declare and set any required Variables.
	// Be sure to periodically call ReportSvcStatus() with SERVICE_START_PENDING.
	// If Initialization failed, then call ReportSvcStatus() with SERVICE_STOPPED.
	// ...


	// Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.
	ghSvcStopEvent = CreateEvent(
                         NULL,    // default security attributes
                         TRUE,    // manual reset event
                         FALSE,   // not signaled
                         NULL);   // no name

	if (ghSvcStopEvent == NULL){
        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }

	// Report running status when initialization is complete.
    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );


	LaunchServiceAction( );

	return;

}


/*
	ServiceMain function.
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms687414%28v=vs.85%29.aspx
*/
void WINAPI ServiceMain(int argc, char ** argv) {


	// first call the RegisterServiceCtrlHandler. Register the SvcHandler function as the service's handler function.
	gSvcStatusHandle = RegisterServiceCtrlHandler( SVCNAME,(LPHANDLER_FUNCTION)&ServiceCtrlHandler);
	if (gSvcStatusHandle == NULL) {		
		//SvcReportEvent(TEXT("RegisterServiceCtrl"));
		// Write in a log file
		return;
	}


	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;


	// Call the ReportSvcStatus function to indicate that its initial status is SERVICE_START_PENDING.
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Calls the SvcInit function to perform the service-specific initialization and begin the work to be performed by the service.
	ServiceInit( );

	//LaunchServiceAction( );

	return;
}


BOOLEAN ServiceLaunchAction( ) {

	SERVICE_TABLE_ENTRY DispatchTable[] = 
    { 
        { SVCNAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain }, 
        { NULL, NULL } 
    }; 


	return TRUE;
}


/*
	ServiceStart()s
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms686315%28v=vs.85%29.aspx
*/
void ServiceLaunch( ) {

	SERVICE_STATUS_PROCESS ssStatus;
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	DWORD dwBytesNeeded;
	DWORD dwOldCheckPoint;
	DWORD dwStartTickCount;
	DWORD dwWaitTime;


	// Get a handle to the SCM
	schSCManager = OpenSCManager(NULL, NULL,SC_MANAGER_ALL_ACCESS) ;
	if (schSCManager == NULL) {
		printf("[-] Error :: ServiceLaunch!OpenSCManager() failed :: exit_code = %d\n",GetLastError());
		return;
	}

	// Get a handle to the srevice.
	schService = OpenService(schSCManager, SVCNAME, SERVICE_ALL_ACCESS);
	if (schService == NULL) {
		printf("[-] Error :: ServiceLaunch!OpenService() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Check the status in case the service is already started.
	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
		printf("[-] Error :: ServiceLaunch!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING ) {

		printf("[i] Warning :: ServiceLaunch :: Cannot start the service because it is already runnnig\n");
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// TODO :: Handle the case where the service is pending to stop.


	// Attempt to start the service.
	if (!StartService(schService,0, NULL)) {
		printf("[-] Error :: ServiceLaunch!StartService() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}
	else 
		printf("[-] Debug :: ServiceLaunch!StartService() :: Service start pending...\n");


	// Check the status until the service is no longer start pending.

	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
		printf("[-] Error :: ServiceLaunch!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	dwStartTickCount = GetTickCount( );
	dwOldCheckPoint = ssStatus.dwCheckPoint;


	while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {


		// Wait a time.
		dwWaitTime = ssStatus.dwWaitHint / 10;

		if (dwWaitTime < 1000) {
			dwWaitTime = 1000;
		}
		else if (dwWaitTime > 10000)
			dwWaitTime = 10000;

		Sleep(dwWaitTime);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
			printf("[-] Error :: ServiceLaunch!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			break;
		}

		if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
			// Continue to wait and check
			dwStartTickCount = GetTickCount( );
			dwOldCheckPoint = ssStatus.dwCheckPoint;
		}
		else {
			if (GetTickCount( ) - dwStartTickCount > ssStatus.dwWaitHint) {

				printf("[i] Warning :: ServiceLaunch :: No progress made within the wait hint\n" );
				// No progress made within the wait hint
				break;
			}
		}
	}

	// Determine whether the service is running.
	if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
        printf("[+] Info :: ServiceLaunch :: Service started successfully !\n" );	

	} else {
		printf("[i] Error :: ServiceLaunch :: Service not started\n" );
		printf("[i] Error :: ServiceLaunch :: Current State: %d\n", ssStatus.dwCurrentState  );
		printf("[i] Error :: ServiceLaunch :: Exit code: %d\n", ssStatus.dwWin32ExitCode );
		printf("[i] Error :: ServiceLaunch :: Check code: %d\n", ssStatus.dwCheckPoint );
		printf("[i] Error :: ServiceLaunch :: Wait Hint: %d\n", ssStatus.dwWaitHint );
    } 

	CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);

	return;
}


/*
	ServiceStop()
	https://msdn.microsoft.com/en-us/library/windows/desktop/ms686335%28v=vs.85%29.aspx
*/
void ServiceStop( ) {

	SERVICE_STATUS_PROCESS ssStatus;
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	DWORD dwBytesNeeded;
	DWORD dwStartTime = GetTickCount( );
	DWORD dwTimeout = 10000;
	DWORD exitCode = 0;


	// Get a handle to the SCM database. 
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (schSCManager == NULL) {
		printf("[-] Error :: ServiceStop!OpenSCManager() failed :: exit_code = %d\n",GetLastError());
		return ;
	}

	// Get a handle to the Service.
	schService = OpenService(schSCManager, SVCNAME, SERVICE_STOP|SERVICE_QUERY_STATUS|SERVICE_ENUMERATE_DEPENDENTS);
	if (schService == NULL) {
		printf("[-] Error :: OpenService() function failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	// Make sure the service is not already stopped.
	if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
		printf("[-] Error :: ServiceStop!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
		printf("[i] Warning :: ServiceStop :: The service is already stopped !\n");
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// If a stop is pending, wait for it.
	// TODO : Set service stop timeout.
	while ( ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
		
		printf("[i] Debug :: ServiceStop :: Service Stop pending...\n");

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
			printf("[-] Error :: ServiceStop!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return;
		}

		if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
			printf("[+] Debug :: ServiceStop :: Service stopped successfully !\n");			
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return;
		}

	}

	// If the service is running, dependencies must be stopped first. (In our case, there is no dependencies).	
	// TODO: Unload databases, + modules + others dependencies...

	// Send a stop code to the service.
	if (!ControlService(schService,SERVICE_CONTROL_STOP,(LPSERVICE_STATUS)&ssStatus)) {
		printf("[-] Error :: ServiceStop!ControlService() failed :: exit_code = %d\n",GetLastError());
		CloseServiceHandle(schService);
		CloseServiceHandle(schSCManager);
		return;
	}

	// Wait for the service to stop.
	while ( ssStatus.dwCurrentState != SERVICE_STOPPED) {

		Sleep(ssStatus.dwWaitHint);

		if (!QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
			printf("[-] Error :: ServiceStop!QueryServiceStatusEx() failed :: exit_code = %d\n",GetLastError());
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return;
		}

		if (ssStatus.dwCurrentState == SERVICE_STOPPED) {			
			break;
		}

		if (GetTickCount( ) - dwStartTime > dwTimeout) {
			printf("[-] Error :: ServiceStop :: Service Stop failed :: Stop Wait timeout !\n");
			CloseServiceHandle(schService);
			CloseServiceHandle(schSCManager);
			return;
		}

	}
	printf("[+] Debug :: Service stopped successfully\n");


	return;
}


int main(int argc, char ** argv) {

	int ret = 0;

	printf("------------------------------\n");
	printf("----- Uhuru Scan service -----\n");
	printf("------------------------------\n");

	// command line parameter "--install", install the service.
	if ( argc >=2 && strncmp(argv[1],"--install",9) == 0 ){

		ret = ServiceInstall( );
		if (ret < 0) {
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;

	}

	// command line parameter "--uninstall", uninstall the service.
	if ( argc >=2 && strncmp(argv[1],"--uninstall",11) == 0 ){

		ret = ServiceRemove( );
		return EXIT_SUCCESS;
	}

	// command line parameter "--remove", delete the service.
	if ( argc >=2 && strncmp(argv[1],"--stop",6) == 0 ){
		ServiceStop();
		return EXIT_SUCCESS;
	}

	if ( argc >=2 && strncmp(argv[1],"--start",7) == 0 ){
		ServiceLaunch( );
		return EXIT_SUCCESS;
	}

	//ServiceLaunchAction( );
	// put this part in ServiceLaunchAction function.
	SERVICE_TABLE_ENTRY DispatchTable[] = 
    { 
        { SVCNAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain }, 
        { NULL, NULL } 
    };


	// This call returs when the service has stopped.
	if (!StartServiceCtrlDispatcher(DispatchTable)) {
		//SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
		//printf("[i] StartServiceCtrlDispatcher :: %d\n",GetLastError());
	}


	return EXIT_SUCCESS;

}