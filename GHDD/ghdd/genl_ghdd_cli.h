/* attributes (variables):
 * the index in this enum is used as a reference for the type,
 * userspace application has to indicate the corresponding type
 * the policy is used for security considerations 
 */
enum {
	GENL_GHDD_CLI_A_UNSPEC,
	GENL_GHDD_CLI_A_MSG,
    __GENL_GHDD_CLI_A_MAX,
};
#define GENL_GHDD_CLI_A_MAX (__GENL_GHDD_CLI_A_MAX - 1)

/* commands: enumeration of all commands (functions), 
 * used by userspace application to identify command to be executed
 */
enum {
	GENL_GHDD_CLI_C_UNSPEC,
	GENL_GHDD_CLI_C_DO,
	__GENL_GHDD_CLI_C_MAX,
};
#define GENL_GHDD_CLI_C_MAX (__GENL_GHDD_CLI_C_MAX - 1)

#define GENL_GHDD_CLI_VERSION 1

#define GENL_GHDD_CLI_FAMILYNAME "GHDD_CLI"

u8 genl_ghdd_cli_init(void);
void genl_ghdd_cli_deinit(void);
int genl_ghdd_cli_send(u8 *mgmt_msg, int len);

