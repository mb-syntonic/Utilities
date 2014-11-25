#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "onep_core_services.h"
#include "onep_constants.h"
#include "onep_vty_constants.h"
#include "onep_vty.h"

static onep_application_name appname = "com.syntonic.app.routerconfig";
static char *configFileName;
static char *ipMask;
static char *allow;

static char* transport = "tls";
static char *app_cert = NULL;
static char *app_private_key = NULL;
static char *app_private_key_password = NULL;
static char *network_element_root_cert = NULL;

static char **routerIPs = NULL;
static char *certFileName = NULL;
static onep_username username;
static onep_password password;

static const char* strRoutersConfigKey = "routers";
static const char* strUserNameConfigKey = "username";
static const char* strPasswordConfigKey = "password";
static const char* strCertFileConfigKey = "certificateFile";

char** str_split(char* a_str, const char a_delim)
{
	char** result = 0;
	size_t count = 0;
	char* tmp = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	count += last_comma < (a_str + strlen(a_str) - 1);

	/* Add space for terminating null string so caller
	knows where the list of returned strings ends. */
	count++;

	result = malloc(sizeof(char*) * count);

	if (result)
	{
		size_t idx = 0;
		char* token = strtok(a_str, delim);

		while (token)
		{
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		*(result + idx) = 0;
	}

	return result;
}

void parse_command_line_and_config(int argc, char* argv[]) {
	
	static const struct option options[] = {
			{ "configFile", required_argument, 0, 'c' },
			{ "ipMask", required_argument, 0, 'i' },
			{ "allow", required_argument, 0, 'a' },
			{ 0 }
	};
	int c, option_index;
	int usage = 0;
	while (1) {
		c = getopt_long(argc, argv, "c:i:a:", options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			configFileName = optarg;
			break;
		case 'i':
			ipMask = optarg;
			break;
		case 'a':
			allow = optarg;
			break;		
		default:
			usage = 1;
			break;
		}
	}
	if ( (argc < 3) || (!configFileName) || (!ipMask) || (!allow) ) {
		usage = 1;
	}
	if (usage) {
		//printf(" args - %d configFilName = %s, ipMask = %s, allow = %s", argc, configFileName, ipMask, allow);
		fprintf(stderr,
			"Usage: %s -c <config file name> "
			"-i <ip or ip mask>",
			"-a <allow or dissallow> ",
			argv[0]);
		exit(1);
	}
	
	FILE *file = fopen(configFileName, "r");
	if (file != NULL)
	{
		char lineFromFile[1000]; /* or other suitable maximum line size */
		while (fgets(lineFromFile, sizeof lineFromFile, file) != NULL) /* read a line */
		{
			char line[1000] = "";
			strncpy(line, lineFromFile, strlen(lineFromFile) - 1);

			char** tokens;
			tokens = str_split(line, '=');
			if (tokens)
			{
				char* key = *(tokens);
				char* value = *(tokens  + 1);

				if (strncmp(strRoutersConfigKey, key, strlen(strRoutersConfigKey) + 1) == 0)
				{
					routerIPs = str_split(value, ',');
				}
				else if (strncmp(strUserNameConfigKey, key, strlen(strUserNameConfigKey) + 1) == 0)
				{
					strcpy(username, value);
				}
				else if (strncmp(strPasswordConfigKey, key, strlen(strPasswordConfigKey) + 1) == 0)
				{
					strcpy(password, value);
				}
				else if (strncmp(strCertFileConfigKey, key, strlen(strCertFileConfigKey) + 1) == 0)
				{
					certFileName = value;
				}
				free(tokens);
			}
		}
		fclose(file);
	}
	else
	{
		perror(configFileName); /* why didn't the file open? */
	}
}

int changeRouterConfig(char* element_hostname)
{
	int ec = EXIT_SUCCESS;
	onep_status_t rc = ONEP_OK;
	onep_network_application_t *nwapp = NULL;
	onep_network_element_t *ne = NULL;
	onep_session_handle_t *sh = NULL;
	onep_element_property_t *property = NULL;
	char *hostname = NULL;
	onep_session_config_t* config = NULL;

	rc = onep_application_get_instance(&nwapp);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to get network application: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
		ec = EXIT_FAILURE;
		goto cleanup;
	}

	rc = onep_application_set_name(nwapp, appname);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to set application name: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
	}

	rc = onep_application_get_network_element_by_name(nwapp,
		element_hostname,
		&ne);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to get network element: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
		ec = EXIT_FAILURE;
		goto cleanup;
	}

	printf("Connecting with onep transport type TLS. \n");
	rc = onep_session_config_new(ONEP_SESSION_TLS, &config);
	if (ONEP_OK != rc) {
		fprintf(stderr, "\nFailed to get config: "
			"errorcode = %d, errormsg = %s",
			rc, onep_strerror(rc));
		(void)onep_session_config_destroy(&config);
		return rc;
	}
	rc = onep_session_config_set_port(config, 15002);
	if (ONEP_OK != rc) {
		fprintf(stderr, "\nFailed to set port: "
			"errorcode = %d, errormsg = %s",
			rc, onep_strerror(rc));
		(void)onep_session_config_destroy(&config);
		return rc;
	}

	rc = onep_session_config_set_tls(
		config, /* Pointer to onep_session_config_t  */
		app_cert, /* Client certificate file path */
		app_private_key,  /* Client private key file path */
		app_private_key_password, /* SSL certificate passcode     */
		network_element_root_cert);  /* Root certificate file path   */

	if (ONEP_OK != rc) {
		fprintf(stderr, "\nFailed to set TLS: errorcode = %d, errormsg = %s",
			rc, onep_strerror(rc));
		if (config)
			(void)onep_session_config_destroy(&config);
		goto disconnect;
		return rc;
	}

	rc = onep_element_connect(ne, username, password, config, &sh);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to connect to network element: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
		ec = EXIT_FAILURE;
		goto cleanup;
	}

	rc = onep_element_get_property(ne, &property);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to get element property: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
		ec = EXIT_FAILURE;
		goto disconnect;
	}

	rc = onep_element_property_get_sys_name(property, &hostname);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to get system name: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
		ec = EXIT_FAILURE;
		goto disconnect;
	}

	onep_vty_t *vty = NULL;
	uint32_t timeout;
	char *response = NULL;

	rc = onep_vty_new(ne, &vty);
	if (rc != ONEP_OK) {
		printf("\nFailed to get vty instance: %d %s", rc, onep_strerror(rc));
		return EXIT_FAILURE;
	}

	rc = onep_vty_open(vty);
	if (rc != ONEP_OK) {
		printf("\nFailed to open vty to Network Element: %d %s", rc,
			onep_strerror(rc));
		return EXIT_FAILURE;
	}

	rc = onep_vty_get_timeout(vty, &timeout);
	if (rc != ONEP_OK) {
		printf("\nFailed to get timeout of vty to Network Element: %d %s", rc,
			onep_strerror(rc));
		return EXIT_FAILURE;
	}

	/* Test command
	char *showCommand = "show access-list DENYACCESS";
	printf("\nWriting a command VTY to the Network Element ... \"%s\"\n",
		showCommand);
	rc = onep_vty_write(vty, showCommand, &response);
	if (rc != ONEP_OK) {
		printf("\nFailed to get response for first from vty: %d %s", rc,
			onep_strerror(rc));
		return EXIT_FAILURE;
	}
	//printf("\n\nResponse for %s is - %s", showCommand, response);
	*/

	char command[1000] = "";
	if ((strncmp(allow, "a", strlen(allow)) == 0) || (strncmp(allow, "A", strlen(allow)) == 0))
		sprintf(command, "configure terminal \r \n ip access-list standard DENYACCESS \r\n no permit %s \r\n end \r\n", ipMask);
	else
		sprintf(command, "configure terminal \r \n ip access-list standard DENYACCESS \r\n permit %s \r\n end \r\n", ipMask);
	//printf("\n\ncommand is - %s\n", command);


	printf("\nWriting a command VTY to the Network Element ... \"%s\"\n",
		command);
	rc = onep_vty_write(vty, command, &response);
	if (rc != ONEP_OK) {
		printf("\nFailed to get response for first from vty: %d %s", rc,
			onep_strerror(rc));
		return EXIT_FAILURE;
	}
	printf("\n\nResponse for %s is - %s", command, response);

	printf("\n Save config changes \n");
	rc = onep_vty_write(vty, "write memory", &response);
	if (rc != ONEP_OK) {
		printf("\nFailed to get response for first from vty: %d %s", rc,
			onep_strerror(rc));
		return EXIT_FAILURE;
	}
	if (hostname)
		free(hostname);

disconnect:
	rc = onep_element_disconnect(ne);
	if (rc != ONEP_OK) {
		fprintf(stderr, "\nFailed to disconnect from network element: "
			"errorcode = %d, errormsg = %s\n\n",
			rc, onep_strerror(rc));
		ec = EXIT_FAILURE;
		goto cleanup;
	}

cleanup:
	if (property)
		(void)onep_element_property_destroy(&property);
	if (sh)
		(void)onep_session_handle_destroy(&sh);
	if (ne)
		(void)onep_element_destroy(&ne);
	if (nwapp)
		(void)onep_application_destroy(&nwapp);

	return ec;
}

/**
*--------------------------------------------------------------
* Main Program
*------------------------------------------------------------------
*/
int main(int argc, char *argv[])
{
	parse_command_line_and_config(argc, argv);

	if (routerIPs)
	{
		int routerIPIdx = 0;
		while (*(routerIPs + routerIPIdx) != NULL)
		{
			//if (routerIPIdx > 0)
			//	printf(" and ");

			char* routerIP = *(routerIPs + routerIPIdx);
			//printf(routerIP);
			
			routerIPIdx++;
		}
	}

	network_element_root_cert = certFileName;

	printf(" --------------------------------------------------------------\n");
	printf(" ------------------   DOING STUFF NOW  ------------------------\n");
	printf(" --------------------------------------------------------------\n");

	int routerIPIdx = 0;
	while (*(routerIPs + routerIPIdx) != NULL)
	{
		char* routerIP = *(routerIPs + routerIPIdx);
		printf(" --------------------------------------------------------------\n");
		printf("issuing commands to router %s \n", routerIP);
		changeRouterConfig(routerIP);
	
		routerIPIdx++;
	}

}
