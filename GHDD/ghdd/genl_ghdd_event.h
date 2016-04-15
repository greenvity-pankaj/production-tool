/* attributes (variables):
 * the index in this enum is used as a reference for the type,
 * userspace application has to indicate the corresponding type
 * the policy is used for security considerations 
 */
enum {
	GENL_GHDD_EVENT_A_UNSPEC,
	GENL_GHDD_EVENT_A_MSG,
    __GENL_GHDD_EVENT_A_MAX,
};
#define GENL_GHDD_EVENT_A_MAX (__GENL_GHDD_EVENT_A_MAX - 1)

/* commands: enumeration of all commands (functions), 
 * used by userspace application to identify command to be executed
 */
enum {
	GENL_GHDD_EVENT_C_UNSPEC,
	GENL_GHDD_EVENT_C_DO,
	__GENL_GHDD_EVENT_C_MAX,
};
#define GENL_GHDD_EVENT_C_MAX (__GENL_GHDD_EVENT_C_MAX - 1)

#define GENL_GHDD_EVENT_VERSION 1

#define GENL_GHDD_EVENT_FAMILYNAME "GHDD_EVENT"
#define GENL_GHDD_EVENT_GROUPNAME "MCPS"

u8 genl_ghdd_event_init(void);
void genl_ghdd_event_deinit(void);
int genl_ghdd_event_send(u8 *mgmt_msg, int len);
