
#ifndef	__CONFIG_COMMON_H__
#define	__CONFIG_COMMON_H__

#include <cJSON.h>

/**
 * @brief Setup XML/JSON의 각 노드별로 처리하기 위한 테이블
 *
 * child 와 handler는 같이 존재 할 수 없다.
 */
struct ConfigHandler {
	const char *name;											/**< XML/JSON 노드의 이름 */
	struct ConfigHandler *child;								/**< 하위 테이블 */
	void (*handler)(cJSON *node, struct PFConfig *config);		/**< 최종 처리 함수 */
};

/* config-system.c */
extern struct ConfigHandler rt_system[];
extern struct ConfigHandler wt_system[];

/* config-network.c */
extern struct ConfigHandler rt_network[];
extern struct ConfigHandler wt_network[];

/* config-media.c */
extern struct ConfigHandler rt_media[];
extern struct ConfigHandler wt_media[];

#endif	/*__CONFIG_COMMON_H__*/
