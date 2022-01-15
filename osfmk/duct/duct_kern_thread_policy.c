#include <duct/duct.h>

#include <duct/duct_pre_xnu.h>
#include <kern/thread.h>
#include <kern/policy_internal.h>
#include <duct/duct_post_xnu.h>

// <copied from="xnu://6153.61.1/osfmk/kern/thread_policy.c">

#define QOS_EXTRACT(q)        ((q) & 0xff)

/*
 * THREAD_QOS_UNSPECIFIED is assigned the highest tier available, so it does not provide a limit
 * to threads that don't have a QoS class set.
 */
const qos_policy_params_t thread_qos_policy_params = {
	/*
	 * This table defines the starting base priority of the thread,
	 * which will be modified by the thread importance and the task max priority
	 * before being applied.
	 */
	.qos_pri[THREAD_QOS_UNSPECIFIED]                = 0, /* not consulted */
	.qos_pri[THREAD_QOS_USER_INTERACTIVE]           = BASEPRI_BACKGROUND, /* i.e. 46 */
	.qos_pri[THREAD_QOS_USER_INITIATED]             = BASEPRI_USER_INITIATED,
	.qos_pri[THREAD_QOS_LEGACY]                     = BASEPRI_DEFAULT,
	.qos_pri[THREAD_QOS_UTILITY]                    = BASEPRI_UTILITY,
	.qos_pri[THREAD_QOS_BACKGROUND]                 = MAXPRI_THROTTLE,
	.qos_pri[THREAD_QOS_MAINTENANCE]                = MAXPRI_THROTTLE,

	/*
	 * This table defines the highest IO priority that a thread marked with this
	 * QoS class can have.
	 */
	.qos_iotier[THREAD_QOS_UNSPECIFIED]             = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_USER_INTERACTIVE]        = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_USER_INITIATED]          = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_LEGACY]                  = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_UTILITY]                 = THROTTLE_LEVEL_TIER1,
	.qos_iotier[THREAD_QOS_BACKGROUND]              = THROTTLE_LEVEL_TIER2, /* possibly overridden by bg_iotier */
	.qos_iotier[THREAD_QOS_MAINTENANCE]             = THROTTLE_LEVEL_TIER3,

	/*
	 * This table defines the highest QoS level that
	 * a thread marked with this QoS class can have.
	 */

	.qos_through_qos[THREAD_QOS_UNSPECIFIED]        = QOS_EXTRACT(THROUGHPUT_QOS_TIER_UNSPECIFIED),
	.qos_through_qos[THREAD_QOS_USER_INTERACTIVE]   = QOS_EXTRACT(THROUGHPUT_QOS_TIER_0),
	.qos_through_qos[THREAD_QOS_USER_INITIATED]     = QOS_EXTRACT(THROUGHPUT_QOS_TIER_1),
	.qos_through_qos[THREAD_QOS_LEGACY]             = QOS_EXTRACT(THROUGHPUT_QOS_TIER_1),
	.qos_through_qos[THREAD_QOS_UTILITY]            = QOS_EXTRACT(THROUGHPUT_QOS_TIER_2),
	.qos_through_qos[THREAD_QOS_BACKGROUND]         = QOS_EXTRACT(THROUGHPUT_QOS_TIER_5),
	.qos_through_qos[THREAD_QOS_MAINTENANCE]        = QOS_EXTRACT(THROUGHPUT_QOS_TIER_5),

	.qos_latency_qos[THREAD_QOS_UNSPECIFIED]        = QOS_EXTRACT(LATENCY_QOS_TIER_UNSPECIFIED),
	.qos_latency_qos[THREAD_QOS_USER_INTERACTIVE]   = QOS_EXTRACT(LATENCY_QOS_TIER_0),
	.qos_latency_qos[THREAD_QOS_USER_INITIATED]     = QOS_EXTRACT(LATENCY_QOS_TIER_1),
	.qos_latency_qos[THREAD_QOS_LEGACY]             = QOS_EXTRACT(LATENCY_QOS_TIER_1),
	.qos_latency_qos[THREAD_QOS_UTILITY]            = QOS_EXTRACT(LATENCY_QOS_TIER_3),
	.qos_latency_qos[THREAD_QOS_BACKGROUND]         = QOS_EXTRACT(LATENCY_QOS_TIER_3),
	.qos_latency_qos[THREAD_QOS_MAINTENANCE]        = QOS_EXTRACT(LATENCY_QOS_TIER_3),
};

/*
 * Convert the thread user promotion base pri to qos for threads in qos world.
 * For priority above UI qos, the qos would be set to UI.
 */
thread_qos_t
thread_user_promotion_qos_for_pri(int priority)
{
	int qos;
	for (qos = THREAD_QOS_USER_INTERACTIVE; qos > THREAD_QOS_MAINTENANCE; qos--) {
		if (thread_qos_policy_params.qos_pri[qos] <= priority) {
			return qos;
		}
	}
	return THREAD_QOS_MAINTENANCE;
}

// </copied>
