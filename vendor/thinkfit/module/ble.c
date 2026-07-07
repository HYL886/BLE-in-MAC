#include "ble.h"
#include "msg.h"
#include "sm.h"
#include "ftms.h"
#include "hal.h"
#include "app_msg.h"

#if(SYS_CONFIG_USE_SDK4_EN == 1)
#include "app_msg.h"
#include "flash_lock.h"
//app_buff
/**
 * @brief	connMaxRxOctets
 * refer to BLE SPEC "4.5.10 Data PDU length management" & "2.4.2.21 LL_LENGTH_REQ and LL_LENGTH_RSP"
 * usage limitation:
 * 1. should be in range of 27 ~ 251
 */
#define ACL_CONN_MAX_RX_OCTETS			27


/**
 * @brief	connMaxTxOctets
 * refer to BLE SPEC "4.5.10 Data PDU length management" & "2.4.2.21 LL_LENGTH_REQ and LL_LENGTH_RSP"
 *  in this SDK, we separate this value into 2 parts: slaveMaxTxOctets and masterMaxTxOctets,
 *  for purpose to save some SRAM costed by when slave and master use different connMaxTxOctets.
 * usage limitation:
 * 1. slaveMaxTxOctets and masterMaxTxOctets should be in range of 27 ~ 251
 */
#define ACL_MASTER_MAX_TX_OCTETS		27
#define ACL_SLAVE_MAX_TX_OCTETS			27
#define ACL_RX_FIFO_SIZE				CAL_LL_ACL_RX_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)
#define ACL_RX_FIFO_NUM					8	// must be: 2^n


/**
 * @brief	ACL TX buffer size & number
 *  		ACL MASTER TX buffer is shared by all master connections to hold LinkLayer RF TX data.
*			ACL SLAVE  TX buffer is shared by all slave  connections to hold LinkLayer RF TX data.
 * usage limitation for ACL_MASTER_TX_FIFO_SIZE & ACL_SLAVE_TX_FIFO_SIZE:
 * 1. should be greater than or equal to (connMaxTxOctets + 10)
 * 2. should be be an integer multiple of 4 (4 Byte align)
 * 3. user can use formula:  size = CAL_LL_ACL_TX_FIFO_SIZE(connMaxTxOctets)
 * usage limitation for ACL_MASTER_TX_FIFO_NUM & ACL_SLAVE_TX_FIFO_NUM:
 * 1. must be: 2^n  (power of 2)
 * 2. at least 8; recommended value: 8, 16, 32; other value not allowed.
 */
#define ACL_MASTER_TX_FIFO_SIZE			CAL_LL_ACL_TX_FIFO_SIZE(ACL_MASTER_MAX_TX_OCTETS)	// ACL_MASTER_MAX_TX_OCTETS + 10, then 4 Byte align
#define ACL_MASTER_TX_FIFO_NUM			8   //different from eagle. 2^n
#define ACL_SLAVE_TX_FIFO_SIZE			CAL_LL_ACL_TX_FIFO_SIZE(ACL_SLAVE_MAX_TX_OCTETS)  // ACL_SLAVE_MAX_TX_OCTETS + 10, then 4 Byte align
#define ACL_SLAVE_TX_FIFO_NUM			8   //different from eagle. 2^n



/***************** ACL connection L2CAP layer MTU TX & RX data FIFO allocation, Begin ********************************/

/*Note:
 * if support LE Secure Connections, L2CAP buffer must >= 72.([64+6]+3)/4*4), 4B align.
 * MTU Buff size = Extra_Len(6)+ ATT_MTU_MAX
 *  1. should be greater than or equal to (ATT_MTU + 6)
 *  2. should be be an integer multiple of 4 (4 Byte align)
 */
#define ATT_MTU_MASTER_RX_MAX_SIZE  23
#define	MTU_M_BUFF_SIZE_MAX			CAL_MTU_BUFF_SIZE(ATT_MTU_MASTER_RX_MAX_SIZE)

#define ATT_MTU_SLAVE_RX_MAX_SIZE   23
#define	MTU_S_BUFF_SIZE_MAX			CAL_MTU_BUFF_SIZE(ATT_MTU_SLAVE_RX_MAX_SIZE)

#if(MCU_CORE_TYPE == MCU_CORE_9518)
_attribute_ble_data_retention_	u8	app_acl_rxfifo[ACL_RX_FIFO_SIZE * ACL_RX_FIFO_NUM] = {0};
#elif(MCU_CORE_TYPE == MCU_CORE_825x || MCU_CORE_TYPE == MCU_CORE_827x)
_attribute_data_no_init_		u8	app_acl_rxfifo[ACL_RX_FIFO_SIZE * ACL_RX_FIFO_NUM] = {0};
#endif


/**
 * @brief	ACL TX buffer. size & number defined in app_buffer.h
 *  ACL MASTER TX buffer should be defined only when ACl connection master role is used.
 *  ACL SLAVE  TX buffer should be defined only when ACl connection slave role is used.
 */
_attribute_ble_data_retention_	u8	app_acl_mstTxfifo[ACL_MASTER_TX_FIFO_SIZE * ACL_MASTER_TX_FIFO_NUM * MASTER_MAX_NUM] = {0};
_attribute_ble_data_retention_	u8	app_acl_slvTxfifo[ACL_SLAVE_TX_FIFO_SIZE * ACL_SLAVE_TX_FIFO_NUM * SLAVE_MAX_NUM] = {0};

/******************** ACL connection LinkLayer TX & RX data FIFO allocation, End ***********************************/

/***************** ACL connection L2CAP layer MTU TX & RX data FIFO allocation, Begin ********************************/
_attribute_ble_data_retention_	u8 mtu_m_rx_fifo[MASTER_MAX_NUM * MTU_M_BUFF_SIZE_MAX];

_attribute_ble_data_retention_	u8 mtu_s_rx_fifo[SLAVE_MAX_NUM * MTU_S_BUFF_SIZE_MAX];
_attribute_ble_data_retention_	u8 mtu_s_tx_fifo[SLAVE_MAX_NUM * MTU_S_BUFF_SIZE_MAX];
/***************** ACL connection L2CAP layer MTU TX & RX data FIFO allocation, End **********************************/


//app_ui
int key_not_released = 0;
int	master_pairing_enable = 0;
int master_unpair_enable = 0;

_attribute_ble_data_retention_ int master_disconnect_connhandle;

int master_auto_connect = 0;
int user_manual_pairing = 0;

//ble
_attribute_ble_data_retention_	int	master_smp_pending = 0; 		// SMP: security & encryption;
_attribute_ble_data_retention_	u8 ota_is_working = 0;

static u8 ble_heartrate_handle = 0x19;



/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int AA_dbg_adv_rpt = 0;
u32	tick_adv_rpt = 0;
u8 master_ccc[2] = {0x01,0x00};

static int main_idle_loop(void);

static u8 searchArrayFromAdv_03030D18(u8 *buf,u8 len)
{
	if(buf == NULL)
	{
		return FALSE;
	}
	u8 ret = false;
	u8 i=0;
	for(i=0;i<=(len-4);i++)
	{
		if(((buf[i] == 0x03)||(buf[i] == 0x05))&&(buf[i+2] == 0x0D)&&(buf[i+3] == 0x18))
		{
			ret = true;
			break;
		}
	}
	return ret;
}

int app_le_adv_report_event_handle(u8 *p)
{
	event_adv_report_t *pa = (event_adv_report_t *)p;
	s8 rssi = pa->data[pa->len];

    if(p[20] == 0x40 && p[21] == 0x58)
    {
        //printf("LE advertising report (rssi:%ddb, len:%d):\n", rssi, pa->len+11);
        //array_printf(p, (pa->len + 11));
        if(rssi > -90)
        {
            ckit_bike_ADpackReceive(pa->mac, &p[20]);
        }
        //printf("%d %d\n",p[23],p[24]);
        //heartrate_set_value(p[25]);
    }

	#if 0  //debug, print ADV report number every 5 seconds
		AA_dbg_adv_rpt ++;
		if(clock_time_exceed(tick_adv_rpt, 5000000)){
			my_dump_str_data(APP_DUMP_EN, "Adv report", pa->mac, 6);
			tick_adv_rpt = clock_time();
		}
	#endif

	/*********************** Master Create connection demo: Key press or ADV pair packet triggers pair  ********************/
	#if (BLE_MASTER_SMP_ENABLE)
		if(master_smp_pending){ 	 //if previous connection SMP not finish, can not create a new connection
			return 1;
		}
	#endif

	#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
		if(master_sdp_pending){ 	 //if previous connection SDP not finish, can not create a new connection
			return 1;
		}
	#endif

	if (master_disconnect_connhandle){ //one master connection is in un_pair disconnection flow, do not create a new one
		return 1;
	}

	master_auto_connect = 0;
	user_manual_pairing = 0;

	//manual pairing methods 1: key press triggers
	//user_manual_pairing = master_pairing_enable && (rssi > -56);  //button trigger pairing(RSSI threshold, short distance)

	if(rssi>-56)
	{
		if(searchArrayFromAdv_03030D18(pa->data,pa->len) == TRUE)
		// if(pa->data[5] == 0x0D && pa->data[6] == 0x18)
		{
			user_manual_pairing = 1;
		}
	}
	
	#if (BLE_MASTER_SMP_ENABLE)
		master_auto_connect = blc_smp_searchBondingSlaveDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
	#else
		//search in slave mac_address table to find whether this device is an old device which has already paired with master
		master_auto_connect = user_tbl_slave_mac_search(pa->adr_type, pa->mac);
	#endif

	if(master_auto_connect || user_manual_pairing){

		/* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
		 * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
		 * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
		 * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
		u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY,  \
								 pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC, \
								 CONN_INTERVAL_100MS, CONN_INTERVAL_100MS, 0, CONN_TIMEOUT_4S, \
								 0, 0xFFFF);


		if(status == BLE_SUCCESS){ //create connection success
			#if (!BLE_MASTER_SMP_ENABLE)
			    // for Telink referenced pair&bonding,
				if(user_manual_pairing && !master_auto_connect){  //manual pair but not auto connect
					blm_manPair.manual_pair = 1;
					blm_manPair.mac_type = pa->adr_type;
					memcpy(blm_manPair.mac, pa->mac, 6);
					blm_manPair.pair_tick = clock_time();
				}
			#endif
		}
	}
	/*********************** Master Create connection demo code end  *******************************************************/


	return 0;
}


/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_complete_event_handle(u8 *p)
{
	hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

	if(pConnEvt->status == BLE_SUCCESS){

		dev_char_info_insert_by_conn_event(pConnEvt);

		if(pConnEvt->role == LL_ROLE_SLAVE){	// slave role
			bls_l2cap_requestConnParamUpdate (pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 99, CONN_TIMEOUT_4S);	// 1 second
//			bls_l2cap_requestConnParamUpdate (pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 199, CONN_TIMEOUT_6S);	// 2 second
//			bls_l2cap_requestConnParamUpdate (pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 299, CONN_TIMEOUT_8S);	// 3 second
		}
		else if(pConnEvt->role == LL_ROLE_MASTER){ // master role, process SMP and SDP if necessary
			#if (BLE_MASTER_SMP_ENABLE)
				master_smp_pending = pConnEvt->connHandle; // this connection need SMP
			#else
				//manual pairing, device match, add this device to slave mac table
				if(blm_manPair.manual_pair && blm_manPair.mac_type == pConnEvt->peerAddrType && !memcmp(blm_manPair.mac, pConnEvt->peerAddr, 6)){
					blm_manPair.manual_pair = 0;
					user_tbl_slave_mac_add(pConnEvt->peerAddrType, pConnEvt->peerAddr);
				}
			#endif



			#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
				memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
				cur_sdp_device.conn_handle = pConnEvt->connHandle;
				cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
				memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

				u8	temp_buff[sizeof(dev_att_t)];
				dev_att_t *pdev_att = (dev_att_t *)temp_buff;

				// /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
				// if( dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att) ){
				// 	//cur_sdp_device.char_handle[1] = 									//Speaker
				// 	cur_sdp_device.char_handle[2] = pdev_att->char_handle[2];			//OTA
				// 	cur_sdp_device.char_handle[3] = pdev_att->char_handle[3];			//consume report
				// 	cur_sdp_device.char_handle[4] = pdev_att->char_handle[4];			//normal key report
				// 	//cur_sdp_device.char_handle[6] =									//BLE Module, SPP Server to Client
				// 	//cur_sdp_device.char_handle[7] =									//BLE Module, SPP Client to Server

				// 	/* add the peer device att_handle value to conn_dev_list */
				// 	dev_char_info_add_peer_att_handle(&cur_sdp_device);
				// }
				// else
				{
					master_sdp_pending = pConnEvt->connHandle;  // mark this connection need SDP

					#if (BLE_MASTER_SMP_ENABLE)
						 //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
					#else
						 app_register_service(&app_service_discovery); 	//No SMP, service discovery can initiated now
					#endif
				}
			#endif
		}
	}

	return 0;
}



/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int 	app_disconnect_event_handle(u8 *p)
{
	event_disconnection_t	*pCon = (event_disconnection_t *)p;

	//terminate reason
	if(pCon->reason == HCI_ERR_CONN_TIMEOUT){  	//connection timeout

	}
	else if(pCon->reason == HCI_ERR_REMOTE_USER_TERM_CONN){  	//peer device send terminate command on link layer
		heartrate_set_value(0);
	}
	//master host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
	else if(pCon->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){
		

	}
	else{

	}


	/* if previous connection SMP & SDP not finished, clear flag */
	#if (BLE_MASTER_SMP_ENABLE)
		if(master_smp_pending == pCon->connHandle){
			master_smp_pending = 0;
		}
	#endif
	#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
		if(master_sdp_pending == pCon->connHandle){
			master_sdp_pending = 0;
		}
	#endif

	if(master_disconnect_connhandle == pCon->connHandle){  //un_pair disconnection flow finish, clear flag
		master_disconnect_connhandle = 0;
	}

	dev_char_info_delete_by_connhandle(pCon->connHandle);


	return 0;
}


/**
 * @brief      BLE Connection update complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_update_complete_event_handle(u8 *p)
{
	hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)p;

	if(pUpt->status == BLE_SUCCESS){

	}

	return 0;
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback (u32 h, u8 *p, int n)
{
	if (h &HCI_FLAG_EVENT_BT_STD)		//Controller HCI event
	{
		u8 evtCode = h & 0xff;

		//------------ disconnect -------------------------------------
		if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)  //connection terminate
		{
			app_disconnect_event_handle(p);
		}
		else if(evtCode == HCI_EVT_LE_META)  //LE Event
		{
			u8 subEvt_code = p[0];

			//------hci le event: le connection complete event---------------------------------
			if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)	// connection complete
			{
				app_le_connection_complete_event_handle(p);
			}
			//--------hci le event: le adv report event ----------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

				app_le_adv_report_event_handle(p);
			}
			//------hci le event: le connection update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
			{
				app_le_connection_update_complete_event_handle(p);
			}
		}
	}


	return 0;

}




/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback (u32 h, u8 *para, int n)
{
	u8 event = h & 0xFF;

	switch(event)
	{
		case GAP_EVT_SMP_PAIRING_BEGIN:
		{

		}
		break;

		case GAP_EVT_SMP_PAIRING_SUCCESS:
		{

		}
		break;

		case GAP_EVT_SMP_PAIRING_FAIL:
		{
			#if (BLE_MASTER_SMP_ENABLE)
				gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;

				if( dev_char_get_conn_role_by_connhandle(p->connHandle) == LL_ROLE_MASTER){  //master connection
					if(master_smp_pending == p->connHandle){
						master_smp_pending = 0;
					}
				}
			#endif
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{

		}
		break;

		case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
		{
			gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;

			if( dev_char_get_conn_role_by_connhandle(p->connHandle) == LL_ROLE_MASTER){  //master connection

				#if (BLE_MASTER_SMP_ENABLE)
					if(master_smp_pending == p->connHandle){
						master_smp_pending = 0;
					}
				#endif

				#if (BLE_MASTER_SIMPLE_SDP_ENABLE)  //SMP finish
					if(master_sdp_pending == p->connHandle){  //SDP is pending
						app_register_service(&app_service_discovery);  //start SDP now
					}
				#endif
			}
		}
		break;

		case GAP_EVT_SMP_TK_DISPALY:
		{

		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
		{

		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_OOB:
		{

		}
		break;

		case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
		{

		}
		break;

		case GAP_EVT_ATT_EXCHANGE_MTU:
		{

		}
		break;

		case GAP_EVT_GATT_HANDLE_VLAUE_CONFIRM:
		{

		}
		break;

		default:
		break;
	}

	return 0;
}



#define			HID_HANDLE_CONSUME_REPORT			25
#define			HID_HANDLE_KEYBOARD_REPORT			29
#define			AUDIO_HANDLE_MIC					52
#define			OTA_HANDLE_DATA						48

/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler (u16 connHandle, u8 *pkt)
{
	if( dev_char_get_conn_role_by_connhandle(connHandle) == LL_ROLE_MASTER )   //GATT data for Master
	{
		#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
			if(master_sdp_pending == connHandle ){  //ATT service discovery is ongoing on this conn_handle
				//when service discovery function is running, all the ATT data from slave
				//will be processed by it,  user can only send your own att cmd after  service discovery is over
				host_att_client_handler (connHandle, pkt); //handle this ATT data by service discovery process
			}
		#endif

		rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

		//so any ATT data before service discovery will be dropped
		dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
		if(dev_info)
		{
			//-------	user process ------------------------------------------------
			u16 attHandle = pAtt->handle;

			// u8 len= pAtt->l2capLen-3;
			// u8 i=0;
			if(attHandle == (ble_heartrate_handle - 1))
			{
				heartrate_set_value(pAtt->dat[1]);
			}
			printf("ble heartrate = %d",pAtt->dat[1]);
			// printf("---opcode=%x attHandle=%x\r\n",pAtt->opcode,attHandle);
			// for(i=0;i<len;i++)
			// 	printf("%02x ",pAtt->dat[i]);
			// printf("\r\n");
			if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //slave handle notify
			{
					//---------------	consumer key --------------------------
				#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
					if(attHandle == dev_info->char_handle[3])  // Consume Report In (Media Key)
				#else
					if(attHandle == HID_HANDLE_CONSUME_REPORT)   //Demo device(825x_ble_sample) Consume Report AttHandle value is 25
				#endif
					{
						//att_keyboard_media (connHandle, pAtt->dat);
					}
					//---------------	keyboard key --------------------------
				#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
					else if(attHandle == dev_info->char_handle[4])     // Key Report In
				#else
					else if(attHandle == HID_HANDLE_KEYBOARD_REPORT)   // Demo device(825x_ble_sample) Key Report AttHandle value is 29
				#endif
					{
						//att_keyboard (connHandle, pAtt->dat);
					}
				#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
					else if(attHandle == dev_info->char_handle[0])     // AUDIO Notify
				#else
					else if(attHandle == AUDIO_HANDLE_MIC)   // Demo device(825x_ble_remote) Key Report AttHandle value is 52
				#endif
					{

					}
					else
					{

					}
			}
			else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
			{
				blc_gatt_pushAttHdlValueCfm(connHandle);
			}
		}

		if(!(pAtt->opcode & 0x01)){
			switch(pAtt->opcode){
				case ATT_OP_FIND_INFO_REQ:
				case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
				case ATT_OP_READ_BY_TYPE_REQ:
				case ATT_OP_READ_BY_GROUP_TYPE_REQ:
					blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_ATTR_NOT_FOUND);
					break;
				case ATT_OP_READ_REQ:
				case ATT_OP_READ_BLOB_REQ:
				case ATT_OP_READ_MULTI_REQ:
				case ATT_OP_WRITE_REQ:
				case ATT_OP_PREPARE_WRITE_REQ:
					blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_INVALID_HANDLE);
					break;
				case ATT_OP_EXECUTE_WRITE_REQ:
				case ATT_OP_HANDLE_VALUE_CFM:
				case ATT_OP_WRITE_CMD:
				case ATT_OP_SIGNED_WRITE_CMD:
					//ignore
					break;
				default://no action
					break;
			}
		}
	}
	else{   //GATT data for Slave


	}


	return 0;
}


void entry_ota_mode(void)
{
    flash_unlock();
	//bls_ota_clearNewFwDataArea();   //must
    sm_set_cur_sta(SM_STA_OTA);
    //bls_ota_setTimeout(240 * 1000 * 1000);
    //ble_sleep_enable = 0;
}

void show_ota_result(int result)
{
    printf("result = %d\n",result);
    if(result == OTA_SUCCESS){
    }
}


/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void	user_set_rf_power (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (RF_POWER_P0dBm);
}



/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
	/* random number generator must be initiated here( in the beginning of user_init_nromal).
	 * When deepSleep retention wakeUp, no need initialize again */
	random_generator_init();

//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

	u8  mac_public[6];
	u8  mac_random_static[6];
	/* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


	//////////// LinkLayer Initialization  Begin /////////////////////////
	blc_ll_initBasicMCU();

	blc_ll_initStandby_module(mac_public);						   //mandatory

    blc_ll_initLegacyAdvertising_module(); 	//adv module: 		 mandatory for BLE slave,

    blc_ll_initLegacyScanning_module(); 	//scan module: 		 mandatory for BLE master

	blc_ll_initInitiating_module();			//initiate module: 	 mandatory for BLE master

	blc_ll_initAclConnection_module();
	blc_ll_initAclMasterRole_module();
	blc_ll_initAclSlaveRole_module();

	blc_ll_setMaxConnectionNumber( MASTER_MAX_NUM, SLAVE_MAX_NUM);

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_MASTER_MAX_TX_OCTETS, ACL_SLAVE_MAX_TX_OCTETS);

	/* all ACL connection share same RX FIFO */
	blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
	/* ACL Master TX FIFO */
	blc_ll_initAclConnMasterTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE, ACL_MASTER_TX_FIFO_NUM, MASTER_MAX_NUM);
	/* ACL Slave TX FIFO */
	blc_ll_initAclConnSlaveTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE, ACL_SLAVE_TX_FIFO_NUM, SLAVE_MAX_NUM);
	//////////// LinkLayer Initialization  End /////////////////////////



	//////////// HCI Initialization  Begin /////////////////////////
	blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler);

	blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

	//bluetooth event
	blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd(		HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
									|	HCI_LE_EVT_MASK_ADVERTISING_REPORT \
									|   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);


	u8 check_status = blc_controller_check_appBufferInitialization();
	if(check_status != BLE_SUCCESS){
		/* here user should set some log to know which application buffer incorrect*/
		write_log32(0x88880000 | check_status);
		while(1);
	}
	//////////// HCI Initialization  End /////////////////////////


	//////////// Host Initialization  Begin /////////////////////////
	/* Host Initialization */
	/* GAP initialization must be done before any other host feature initialization !!! */
	blc_gap_init();

	/* L2CAP buffer Initialization */
	blc_l2cap_initAclConnMasterMtuBuffer(mtu_m_rx_fifo, MTU_M_BUFF_SIZE_MAX, 			0,					 0);
	blc_l2cap_initAclConnSlaveMtuBuffer(mtu_s_rx_fifo, MTU_S_BUFF_SIZE_MAX, mtu_s_tx_fifo, MTU_S_BUFF_SIZE_MAX);

	blc_att_setMasterRxMTUSize(ATT_MTU_MASTER_RX_MAX_SIZE); ///must be placed after "blc_gap_init"
	blc_att_setSlaveRxMTUSize(ATT_MTU_SLAVE_RX_MAX_SIZE);   ///must be placed after "blc_gap_init"

	/* GATT Initialization */
	extern void my_att_init(void);
	my_att_init();
	#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
		host_att_register_idle_func (main_idle_loop);
	#endif
	blc_gatt_register_data_handler(app_gatt_data_handler);

	/* SMP Initialization */
	#if (BLE_SLAVE_SMP_ENABLE || BLE_MASTER_SMP_ENABLE)
		/* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
		blc_smp_configPairingSecurityInfoStorageAddressAndSize(FLASH_ADR_SMP_PAIRING, FLASH_SMP_PAIRING_MAX_SIZE);
	#endif

	#if (BLE_SLAVE_SMP_ENABLE)  //Slave SMP Enable
		blc_smp_setSecurityLevel_slave(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
	#else
		blc_smp_setSecurityLevel_slave(No_Security);
	#endif

	#if (BLE_MASTER_SMP_ENABLE) //Master SMP Enable
		blc_smp_setSecurityLevel_master(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
	#else
		blc_smp_setSecurityLevel_master(No_Security);
		user_master_host_pairing_management_init(); 		//TeLink referenced pairing&bonding without standard pairing in BLE Spec
	#endif

	blc_smp_smpParamInit();


	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN 			|  \
						  GAP_EVT_MASK_SMP_PAIRING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PAIRING_FAIL				|  \
						  GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
	//////////// Host Initialization  End /////////////////////////

//////////////////////////// BLE stack Initialization  End //////////////////////////////////




//////////////////////////// User Configuration for BLE application ////////////////////////////
	hal_init_ble_data();
	hal_set_ble_data();
	blc_ll_setAdvParam(ADV_INTERVAL_200MS, ADV_INTERVAL_200MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable
	blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);

	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_50MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

	user_set_rf_power(0, 0, 0);


	#if (BLE_APP_PM_ENABLE)
		blc_ll_initPowerManagement_module();
		blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_SLAVE | PM_SLEEP_ACL_MASTER);

		#if (PM_DEEPSLEEP_RETENTION_ENABLE)
			blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Enable);
			blc_pm_setDeepsleepRetentionThreshold(95);

			#if(MCU_CORE_TYPE == MCU_CORE_825x)
				blc_pm_setDeepsleepRetentionEarlyWakeupTiming(260);
			#elif(MCU_CORE_TYPE == MCU_CORE_827x)
				blc_pm_setDeepsleepRetentionEarlyWakeupTiming(350);
			#endif
		#else
			blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Disable);
		#endif

		blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);
	#endif


	#if (BLE_OTA_SERVER_ENABLE)
		blc_ota_initOtaServer_module();
		blc_ota_registerOtaStartCmdCb(entry_ota_mode);
		blc_ota_registerOtaResultIndicationCb(show_ota_result);
		blc_ota_setOtaProcessTimeout(240);
	#endif
////////////////////////////////////////////////////////////////////////////////////////////////

}



/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)
	blc_ll_initBasicMCU();   //mandatory

	user_set_rf_power(0, 0, 0);

	blc_ll_recoverDeepRetention();

	DBG_CHN0_HIGH;    //debug
	irq_enable();

	#if (UI_KEYBOARD_ENABLE)
		/////////// keyboard GPIO wakeup init ////////
		u32 pin[] = KB_DRIVE_PINS;
		for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++){
			cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high level wakeup deepsleep
		}
	#endif
#endif
}




void app_process_power_management(void)
{
#if (BLE_APP_PM_ENABLE)
	blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_SLAVE | PM_SLEEP_ACL_MASTER);

	int user_task_flg = ota_is_working;
	#if UI_KEYBOARD_ENABLE
		user_task_flg = user_task_flg || scan_pin_need || key_not_released;
	#endif
	#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
		user_task_flg = user_task_flg || master_sdp_pending;
	#endif


	if(user_task_flg){
		blc_pm_setSleepMask(PM_SLEEP_DISABLE);
	}
#endif
}

////////////////////
//设置蓝牙臂带的心率2a37对应的2902为0001
//////////
void ble_heartrate_set_hande(u8 handle)
{
	ble_heartrate_handle = handle;
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
static u32 delay_ticks = 0;
// static u8 blc_status = 0;
int main_idle_loop (void)
{

	////////////////////////////////////// BLE entry /////////////////////////////////
	blc_sdk_main_loop();

	if(clock_time_exceed(delay_ticks,1000*1000))
	{
		if(BLE_SUCCESS == blc_gatt_pushWriteRequest(0x80,ble_heartrate_handle,master_ccc,2))//(conn_dev_list[1].conn_handle
		{
			// printf("WriteCommand\n");
		}
		delay_ticks = clock_time();
	}

	////////////////////////////////////// PM entry /////////////////////////////////
	app_process_power_management();

	return 0; //must return 0 due to SDP flow
}



/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void ble_pm_proc(void)
{
	main_idle_loop ();

	#if (BLE_MASTER_SIMPLE_SDP_ENABLE)
		simple_sdp_loop ();
	#endif
}

static u8 ble_receive_buf[30];
int ble_com_receive_callback(u16 connHandle, rf_packet_att_t *p)
{
	rf_packet_att_t *pw = p;
    int len = pw->l2capLen - 3;
	memcpy(ble_receive_buf,(u8 *)&pw->dat,len);
	
    ftms_set_delay();
    msg_ble_receive(ble_receive_buf,len);

    return 0;
}

void ble_init(int deepRetWakeUp)
{
	if(deepRetWakeUp)
	{
		user_init_deepRetn();
	}else{
		user_init_normal();
	}
}

void ble_set_power_off(void)
{
	
}

#else
#define 	BLE_DIRECT_ADV_TIME					2000000

#define     BLE_USE_AD_PACK	1
#define 	BLE_APP_ADV_CHANNEL					BLT_ENABLE_ADV_ALL
#define 	BLE_ADV_INTERVAL_MIN				ADV_INTERVAL_30MS
#define 	BLE_ADV_INTERVAL_MAX				ADV_INTERVAL_35MS

#define		BLE_RF_POWER_INDEX					RF_POWER_P3dBm

#define		BLE_DEVICE_ADDRESS_TYPE 			BLE_DEVICE_ADDRESS_PUBLIC


_attribute_data_retention_	u8 ota_is_working = 0;
_attribute_data_retention_	own_addr_type_t 	app_own_address_type = OWN_ADDRESS_PUBLIC;

/**
 * @brief      LinkLayer RX & TX FIFO configuration
 */
/* CAL_LL_ACL_RX_BUF_SIZE(maxRxOct): maxRxOct + 22, then 16 byte align */
#define RX_FIFO_SIZE	64
/* must be: 2^n, (power of 2);at least 4; recommended value: 4, 8, 16 */
#define RX_FIFO_NUM		8


/* CAL_LL_ACL_TX_BUF_SIZE(maxTxOct):  maxTxOct + 10, then 4 byte align */
#define TX_FIFO_SIZE	40
/* must be: (2^n), (power of 2); at least 8; recommended value: 8, 16, 32, other value not allowed. */
#define TX_FIFO_NUM		16


_attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_rxfifo = {
												RX_FIFO_SIZE,
												RX_FIFO_NUM,
												0,
												0,
												blt_rxfifo_b,};


_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_txfifo = {
												TX_FIFO_SIZE,
												TX_FIFO_NUM,
												0,
												0,
												blt_txfifo_b,};


_attribute_data_retention_	int device_in_connection_state;

_attribute_data_retention_	u8	sendTerminate_before_enterDeep = 0;

_attribute_data_retention_	u32	latest_user_event_tick;

_attribute_data_retention_ 	u32 ble_sleep_tick = BLE_POWEROFF_ENTER_DEEP_TIME;

/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_ENTER"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void task_sleep_enter (u8 e, u8 *p, int n)
{
	(void)e;(void)p;(void)n;
	if( blc_ll_getCurrentState() == BLS_LINK_STATE_CONN && ((u32)(bls_pm_getSystemWakeupTick() - clock_time())) > 80 * SYSTEM_TIMER_TICK_1MS){  //suspend time > 30ms.add gpio wakeup
		bls_pm_setWakeupSource(PM_WAKEUP_PAD);  //gpio pad wakeup suspend/deepsleep
	}
}

/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_ADV_DURATION_TIMEOUT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void app_switch_to_undirected_adv(u8 e, u8 *p, int n)
{
	(void)e;(void)p;(void)n;
	bls_ll_setAdvParam( BLE_ADV_INTERVAL_MIN, BLE_ADV_INTERVAL_MAX,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
						0,  NULL,
						BLE_APP_ADV_CHANNEL,
						ADV_FP_NONE);

	/* clear resolving list:
	 * 1. delete all devices in resolving list.
	 * 2. disable address resolution */
	blc_ll_clearResolvingList();

	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //must: set ADV enable
}

/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_CONNECT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void	task_connect (u8 e, u8 *p, int n)
{
	(void)e;(void)p;(void)n;
	//tlk_contr_evt_connect_t *pConnEvt = (tlk_contr_evt_connect_t *)p;
	//tlkapi_send_string_data(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] connect, intA & advA:", pConnEvt->initA, 12);
//	bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 19, CONN_TIMEOUT_4S);  // 200mS
	bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 99, CONN_TIMEOUT_4S);  // 1 S
//	bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 149, CONN_TIMEOUT_8S);  // 1.5 S
//	bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 199, CONN_TIMEOUT_8S);  // 2 S
//	bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 249, CONN_TIMEOUT_8S);  // 2.5 S
//	bls_l2cap_requestConnParamUpdate (CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 299, CONN_TIMEOUT_8S);  // 3 S

	latest_user_event_tick = clock_time();

	device_in_connection_state = 1;//
}

/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_TERMINATE"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void 	task_terminate(u8 e, u8 *p, int n) //*p is terminate reason
{
	(void)e;(void)n;


	device_in_connection_state = 0;


	tlk_contr_evt_terminate_t *pEvt = (tlk_contr_evt_terminate_t *)p;
	if(pEvt->terminate_reason == HCI_ERR_CONN_TIMEOUT){

	}
	else if(pEvt->terminate_reason == HCI_ERR_REMOTE_USER_TERM_CONN){

	}
	else if(pEvt->terminate_reason == HCI_ERR_CONN_TERM_MIC_FAILURE){

	}
	else{

	}

	//tlkapi_printf(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] disconnect, reason 0x%x\n", pEvt->terminate_reason);

#if (BLE_APP_PM_ENABLE)
	 //user has push terminate packet to BLE TX buffer before deepsleep
	if(sendTerminate_before_enterDeep == 1){
		sendTerminate_before_enterDeep = 2;
		bls_ll_setAdvEnable(BLC_ADV_DISABLE);   //disable ADV
	}
#endif

	ble_keep_alive();
}

/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void task_suspend_exit (u8 e, u8 *p, int n)
{
	(void)e;(void)p;(void)n;
	rf_set_power_level_index (BLE_RF_POWER_INDEX);
}


/**
 * @brief      callback function of LinkLayer Event "BLT_EV_FLAG_DATA_LENGTH_EXCHANGE"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
void	task_dle_exchange (u8 e, u8 *p, int n)
{
	//tlk_contr_evt_dataLenExg_t* pEvt = (tlk_contr_evt_dataLenExg_t*)p;
	//tlkapi_send_string_data(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] DLE exchange", &pEvt->connEffectiveMaxRxOctets, 4);
}


#if (BLE_USE_AD_PACK == 1)
int controller_event_callback (u32 h, u8 *p, int n)
{
	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

				event_adv_report_t *pa = (event_adv_report_t *)p;
				s8 rssi = (s8)pa->data[pa->len];//rssi has already plus 110.
				if(p[20] == 0x40 && p[21] == 0x58)
				{
					//printf("LE advertising report (rssi:%ddb, len:%d):\n", rssi, pa->len+11);
					//array_printf(p, (pa->len + 11));
					if(rssi > -90)
					{
						ckit_bike_ADpackReceive(pa->mac, &p[20]);
					}
					//printf("%d %d\n",p[23],p[24]);
					//heartrate_set_value(p[25]);
				}

				#if (DBG_ADV_REPORT_ON_RAM)
					if(pa->len > 31){
						pa->len = 31;
					}
					memcpy( (u8 *)AA_advRpt[AA_advRpt_index++],  p, pa->len + 11);
					if(AA_advRpt_index >= RAM_ADV_MAX){
						AA_advRpt_index = 0;
					}
				#endif
			}
		}
	}

}

void ble_scanning_init(void)
{
	u8 mac_public[6];
    u8 mac_random_static[6];
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
	blc_ll_initScanning_module(mac_public);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);
	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x62, 0xE1, 0x82, 0xC2, 0x50, 0xC7};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif
	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role
	printf("AD pack receive ready!\n");
}	

#endif

/**
 * @brief      callback function of Host Event
 * @param[in]  h - Host Event type
 * @param[in]  para - data pointer of event
 * @param[in]  n - data length of event
 * @return     0
 */
int app_host_event_callback (u32 h, u8 *para, int n)
{

	u8 event = h & 0xFF;

	switch(event)
	{
		case GAP_EVT_SMP_PAIRING_BEGIN:
		{
			//gap_smp_pairingBeginEvt_t *pEvt = (gap_smp_pairingBeginEvt_t *)para;
			//tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring begin:", pEvt, sizeof(gap_smp_pairingBeginEvt_t));
		}
		break;

		case GAP_EVT_SMP_PAIRING_SUCCESS:
		{
			//gap_smp_pairingSuccessEvt_t *pEvt = (gap_smp_pairingSuccessEvt_t *)para;
			//tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring success:", pEvt, sizeof(gap_smp_pairingSuccessEvt_t));
		}
		break;

		case GAP_EVT_SMP_PAIRING_FAIL:
		{
			//gap_smp_pairingFailEvt_t *pEvt = (gap_smp_pairingFailEvt_t *)para;
			//tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring fail:", pEvt, sizeof(gap_smp_pairingFailEvt_t));
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{
			//gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
		}
		break;

		case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
		{
			//gap_smp_securityProcessDoneEvt_t *pEvt = (gap_smp_securityProcessDoneEvt_t *)para;
		}
		break;


		case GAP_EVT_SMP_TK_DISPLAY:
		{
			//u32 pinCode = MAKE_U32(para[3], para[2], para[1], para[0]);
		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
		{
			//for this event, no data, "para" is NULL
		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_OOB:
		{
			//for this event, no data, "para" is NULL
		}
		break;

		case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
		{
			//u32 pinCode = MAKE_U32(para[3], para[2], para[1], para[0]);
		}
		break;

		case GAP_EVT_ATT_EXCHANGE_MTU:
		{
			//gap_gatt_mtuSizeExchangeEvt_t *pEvt = (gap_gatt_mtuSizeExchangeEvt_t *)para;
			//tlkapi_send_string_data(APP_HOST_EVENT_LOG_EN, "[APP][MTU] mtu exchange", pEvt, sizeof(gap_gatt_mtuSizeExchangeEvt_t));
		}
		break;

		case GAP_EVT_GATT_HANDLE_VALUE_CONFIRM:
		{
			//for this event, no data, "para" is NULL
		}
		break;


		default:
		break;
	}

	return 0;
}


void ble_enter_sleep_mode(void)
{
    sm_deinit();
    cpu_sleep_wakeup(DEEPSLEEP_MODE_RET_SRAM_LOW32K, PM_WAKEUP_PAD, 0);  //deepsleep
}

/**
 * @brief      power management code for application
 * @param	   none
 * @return     none
 */
void ble_pm_proc(void)
{
#if(BLE_APP_PM_ENABLE)
	if(blc_ll_getCurrentState() == BLS_LINK_STATE_IDLE){ //PM module can not manage Idle state low power.
		/* user manage BLE Idle state sleep with API "cpu_sleep_wakeup" */
		if(sendTerminate_before_enterDeep == 2){  //Terminate OK
			analog_write(USED_DEEP_ANA_REG, analog_read(USED_DEEP_ANA_REG) | CONN_DEEP_FLG);
			sendTerminate_before_enterDeep = 0;
			ble_enter_sleep_mode();  //deepSleep
		}
	}
	else{ //PM module manage advertising and ACL connection Slave role low power only

		#if (PM_DEEPSLEEP_RETENTION_ENABLE)
			bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
		#else
			bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
		#endif


		//do not care about keyScan/button_detect power here, if you care about this, please refer to "B85m_ble_remote" demo
			if(ota_is_working){
				bls_pm_setManualLatency(0);
			}

			if(!ota_is_working && !blc_ll_isControllerEventPending()){  //no controller event pending
				/* enter deepsleep mode after advertising for 60 seconds without being connected. */
				if( blc_ll_getCurrentState() == BLS_LINK_STATE_ADV && !sendTerminate_before_enterDeep && \
					clock_time_exceed(ble_get_alive_tick() , ble_get_sleep_tick() * 1000000)){
					cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
				}
				/* enter deepsleep mode after 60 seconds without any UI action(key/voice/led) in connection state. */
				else if( device_in_connection_state && \
						clock_time_exceed(ble_get_alive_tick(), ble_get_sleep_tick() * 1000000) ){
					bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN); //push terminate command into BLE TX buffer
					sendTerminate_before_enterDeep = 1;
				}
			}
	}
#endif  //end of BLE_APP_PM_ENABLE
}

void entry_ota_mode(void)
{
    sm_set_cur_sta(SM_STA_OTA);
    //bls_ota_setTimeout(240 * 1000 * 1000);
    ble_set_ota_tick();
}

void show_ota_result(int result)
{
    if(result == OTA_SUCCESS){
    }
}

void ble_set_scanRsp(u8 *pid,u8* model)
{

}

int ble_l2cap_packet_receive(u16 connHandle, u8 * p)
{
    int ret = 0;
    ret = blc_l2cap_packet_receive(connHandle,p);
    //l2cap data packeted, make sure that user see complete l2cap data
    rf_packet_l2cap_t *ptrL2cap = blm_l2cap_packet_pack (connHandle, p);
    if (!ptrL2cap)
    {
        //printf("ptrL2cap null.\n");
        return ret;
    }

	//l2cap data channel id, 4 for ATT, 5 for Signal, 6 for SMP
	if(ptrL2cap->chanId == L2CAP_CID_ATTR_PROTOCOL)  //att data
	{
        rf_packet_att_t *pAtt = (rf_packet_att_t*)ptrL2cap;
        u16 attHandle = pAtt->handle0 | pAtt->handle1<<8;
        if(attHandle == TREADMILL_2ACD_CCB_H || attHandle == BIKE_2AD2_CCB_H)
        {
            if(pAtt->dat[0] == 0x01)
            {
                ftms_start();
            }else if(pAtt->dat[0] == 0x00)
			{
				ftms_stop();
			}
        }
        if(attHandle == COMM_FFF1_DESC_H || attHandle == COMM_FFF1_CCB_H)
        {
            if(pAtt->opcode == 18)
            {
                //msg_set_delay();
                //module_vble_set_delay();
            }
        }
    }
    return ret;
}

#if (APP_FLASH_PROTECTION_ENABLE)

/**
 * @brief      flash protection operation, including all locking & unlocking for application
 * 			   handle all flash write & erase action for this demo code. use should add more more if they have more flash operation.
 * @param[in]  flash_op_evt - flash operation event, including application layer action and stack layer action event(OTA write & erase)
 * 			   attention 1: if you have more flash write or erase action, you should should add more type and process them
 * 			   attention 2: for "end" event, no need to pay attention on op_addr_begin & op_addr_end, we set them to 0 for
 * 			   			    stack event, such as stack OTA write new firmware end event
 * @param[in]  op_addr_begin - operating flash address range begin value
 * @param[in]  op_addr_end - operating flash address range end value
 * 			   attention that, we use: [op_addr_begin, op_addr_end)
 * 			   e.g. if we write flash sector from 0x10000 to 0x20000, actual operating flash address is 0x10000 ~ 0x1FFFF
 * 			   		but we use [0x10000, 0x20000):  op_addr_begin = 0x10000, op_addr_end = 0x20000
 * @return     none
 */
_attribute_data_retention_ u16  flash_lockBlock_cmd = 0;
void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end)
{
	if(flash_op_evt == FLASH_OP_EVT_APP_INITIALIZATION)
	{
		/* ignore "op addr_begin" and "op addr_end" for initialization event
		 * must call "flash protection_init" first, will choose correct flash protection relative API according to current internal flash type in MCU */
		flash_protection_init();

		/* just sample code here, protect all flash area for old firmware and OTA new firmware.
		 * user can change this design if have other consideration */
		u32  app_lockBlock = 0;
		#if (BLE_OTA_SERVER_ENABLE)
			u32 multiBootAddress = blc_ota_getCurrentUsedMultipleBootAddress();
			if(multiBootAddress == MULTI_BOOT_ADDR_0x20000){
				app_lockBlock = FLASH_LOCK_FW_LOW_256K;
			}
			else if(multiBootAddress == MULTI_BOOT_ADDR_0x40000){
				/* attention that 512K capacity flash can not lock all 512K area, should leave some upper sector
				 * for system data(SMP storage data & calibration data & MAC address) and user data
				 * will use a approximate value */
				app_lockBlock = FLASH_LOCK_FW_LOW_512K;
			}
			#if(MCU_CORE_TYPE == MCU_CORE_827x)
			else if(multiBootAddress == MULTI_BOOT_ADDR_0x80000){
				if(blc_flash_capacity < FLASH_SIZE_1M){ //for flash capacity smaller than 1M, OTA can not use 512K as multiple boot address
					blc_flashProt.init_err = 1;
				}
				else{
					/* attention that 1M capacity flash can not lock all 1M area, should leave some upper sector for
					 * system data(SMP storage data & calibration data & MAC address) and user data
					 * will use a approximate value */
					app_lockBlock = FLASH_LOCK_FW_LOW_1M;
				}
			}
			#endif
		#else
			app_lockBlock = FLASH_LOCK_FW_LOW_256K; //just demo value, user can change this value according to application
		#endif


		flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);

		if(blc_flashProt.init_err){
			//tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] flash protection initialization error!!!\n");
		}

		//tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] initialization, lock flash\n");
		flash_lock(flash_lockBlock_cmd);
	}
#if (BLE_OTA_SERVER_ENABLE)
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_BEGIN)
	{
		/* OTA clear old firmware begin event is triggered by stack, in "blc ota_initOtaServer_module", rebooting from a successful OTA.
		 * Software will erase whole old firmware for potential next new OTA, need unlock flash if any part of flash address from
		 * "op addr_begin" to "op addr_end" is in locking block area.
		 * In this sample code, we protect whole flash area for old and new firmware, so here we do not need judge "op addr_begin" and "op addr_end",
		 * must unlock flash */
		//tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA clear old FW begin, unlock flash\n");
		flash_unlock();
	}
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_END)
	{
		/* ignore "op addr_begin" and "op addr_end" for END event
		 * OTA clear old firmware end event is triggered by stack, in "blc ota_initOtaServer_module", erasing old firmware data finished.
		 * In this sample code, we need lock flash again, because we have unlocked it at the begin event of clear old firmware */
		//tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA clear old FW end, restore flash locking\n");
		flash_lock(flash_lockBlock_cmd);
	}
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_BEGIN)
	{
		/* OTA write new firmware begin event is triggered by stack, when receive first OTA data PDU.
		 * Software will write data to flash on new firmware area,  need unlock flash if any part of flash address from
		 * "op addr_begin" to "op addr_end" is in locking block area.
		 * In this sample code, we protect whole flash area for old and new firmware, so here we do not need judge "op addr_begin" and "op addr_end",
		 * must unlock flash */
		//tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA write new FW begin, unlock flash\n");
		flash_unlock();
	}
	else if(flash_op_evt == FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END)
	{
		/* ignore "op addr_begin" and "op addr_end" for END event
		 * OTA write new firmware end event is triggered by stack, after OTA end or an OTA error happens, writing new firmware data finished.
		 * In this sample code, we need lock flash again, because we have unlocked it at the begin event of write new firmware */
		//tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA write new FW end, restore flash locking\n");
		flash_lock(flash_lockBlock_cmd);
	}
#endif
	/* add more flash protection operation for your application if needed */
}


#endif

/**
 * @brief		user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]	none
 * @return      none
 */
_attribute_no_inline_ void ble_init_normal(void)
{

//////////////////////////// basic hardware Initialization  Begin //////////////////////////////////

	/* random number generator must be initiated before any BLE stack initialization.
	 * When deepSleep retention wakeUp, no need initialize again */
	random_generator_init();
	
	blc_readFlashSize_autoConfigCustomFlashSector();

	/* attention that this function must be called after "blc_readFlashSize_autoConfigCustomFlashSector" !!!*/
	blc_app_loadCustomizedParameters_normal();
	
#if (APP_FLASH_PROTECTION_ENABLE)
	app_flash_protection_operation(FLASH_OP_EVT_APP_INITIALIZATION, 0, 0);
	blc_appRegisterStackFlashOperationCallback(app_flash_protection_operation); //register flash operation callback for stack
#endif
//////////////////////////// basic hardware Initialization  End //////////////////////////////////

//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////
	//////////// Controller Initialization  Begin /////////////////////////
	u8  mac_public[6];
	u8  mac_random_static[6];
	/* for 512K Flash, flash_sector_mac_address equals to 0x76000, for 1M  Flash, flash_sector_mac_address equals to 0xFF000 */
	blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);
	//tlkapi_send_string_data(APP_LOG_EN,"[APP][INI]Public Address", mac_public, 6);

	#if(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_PUBLIC)
		app_own_address_type = OWN_ADDRESS_PUBLIC;
	#elif(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_RANDOM_STATIC)
		app_own_address_type = OWN_ADDRESS_RANDOM;
		blc_ll_setRandomAddr(mac_random_static);
	#endif

	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(mac_public);		//mandatory
	blc_ll_initAdvertising_module(mac_public); 	//legacy advertising module: mandatory for BLE slave
	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,
	//////////// Controller Initialization  End /////////////////////////

	//////////// Host Initialization  Begin /////////////////////////
	/* Host Initialization */
	/* GAP initialization must be done before any other host feature initialization !!! */
	blc_gap_peripheral_init();    //gap initialization
	blc_l2cap_register_handler (ble_l2cap_packet_receive);  	//l2cap initialization
	hal_init_ble_data();
	extern void my_att_init(void);
	my_att_init(); //gatt initialization
	blc_att_setRxMtuSize(MTU_SIZE_SETTING); //set MTU size, default MTU is 23 if not call this API

	/* SMP Initialization may involve flash write/erase(when one sector stores too much information,
	 *   is about to exceed the sector threshold, this sector must be erased, and all useful information
	 *   should re_stored) , so it must be done after battery check */
#if (BLE_APP_SECURITY_ENABLE)
	/* attention: If this API is used, must be called before "blc smp_peripheral_init" when initialization !!! */
	bls_smp_configPairingSecurityInfoStorageAddr(flash_sector_smp_storage);
	blc_smp_peripheral_init();

	/* Hid device on android7.0/7.1 or later version
	 * New paring: send security_request immediately after connection complete
	 * reConnect:  send security_request 1000mS after connection complete. If master start paring or encryption before 1000mS timeout, slave do not send security_request. */
	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection)
#else
	blc_smp_setSecurityLevel(No_Security);
#endif


	/* host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask */
	blc_gap_registerHostEventHandler(app_host_event_callback);
	/* enable some frequently-used host event by default, user can add more host event */
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN 			|  \
						  GAP_EVT_MASK_SMP_PAIRING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PAIRING_FAIL				|  \
						  GAP_EVT_MASK_ATT_EXCHANGE_MTU);
	//////////// Host Initialization  End /////////////////////////
	
	//////////// Service Initialization  Begin /////////////////////////
#if (BLE_OTA_SERVER_ENABLE)
	////////////////// OTA relative ////////////////////////
	blc_ota_initOtaServer_module();

	blc_ota_setOtaProcessTimeout(BLE_OTA_ENTER_DEEP_TIME);   //OTA process timeout:  240 seconds
	blc_ota_setOtaDataPacketTimeout(4);	//OTA data packet timeout:  4 seconds
	blc_ota_registerOtaStartCmdCb(entry_ota_mode);
	blc_ota_registerOtaResultIndicationCb(show_ota_result);
#endif
	//////////// Service Initialization  End   /////////////////////////

//////////////////////////// BLE stack Initialization  End //////////////////////////////////

//////////////////////////// User Configuration for BLE application ////////////////////////////
	////////////////// config ADV packet /////////////////////
	u8 adv_param_status = BLE_SUCCESS;
	#if (BLE_APP_SECURITY_ENABLE)
		u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber();  //get bonded device number
		smp_param_save_t  bondInfo;
		if(bond_number)   //at least 1 bonding device exist
		{
			bls_smp_param_loadByIndex( bond_number - 1, &bondInfo);  //get the latest bonding device (index: bond_number-1 )

		}

		if(bond_number)   //set direct ADV
		{
			/* set direct ADV
			 * bondInfo.peer_addr_type & bondInfo.peer_addr is the address in the air packet of "CONNECT_IND" PDU stored in Flash.
			 * if peer address is IDA(identity address), bondInfo.peer_addr is OK used here.
			 * if peer address is RPA(resolved private address), bondInfo.peer_addr is one RPA peer device has used, it has a correct relation
			 * with peer IRK, so it can match to peer device at any time even peer device changes it's RPA. */
			adv_param_status = bls_ll_setAdvParam( BLE_ADV_INTERVAL_MIN, BLE_ADV_INTERVAL_MAX,
											ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY, app_own_address_type,
											bondInfo.peer_addr_type,  bondInfo.peer_addr,
											BLE_APP_ADV_CHANNEL,
											ADV_FP_NONE);

			/* If IRK distributed by peer device is valid, peer device may use RPA(resolved private address) at any time,
			 * even if it used IDA(identity address) in first pairing phase.
			 * So here must add peer IRK to resolving list and enable address resolution, since local device should check if
			 * "CONNECT_IND" PDU is sent by the device directed to.
			 * attention: local RPA not used, so parameter "local_irk" set to NULL */
			if(blc_app_isIrkValid(bondInfo.peer_irk)){
				blc_ll_addDeviceToResolvingList(bondInfo.peer_id_adrType, bondInfo.peer_id_addr, bondInfo.peer_irk, NULL);
				blc_ll_setAddressResolutionEnable(1);
			}

			//it is recommended that direct ADV only last for several seconds, then switch to undirected adv
			bls_ll_setAdvDuration(BLE_DIRECT_ADV_TIME, 1);
			bls_app_registerEventCallback (BLT_EV_FLAG_ADV_DURATION_TIMEOUT, &app_switch_to_undirected_adv);

		}
		else   //set undirected adv
	#endif
		{
			adv_param_status = bls_ll_setAdvParam(  BLE_ADV_INTERVAL_MIN, BLE_ADV_INTERVAL_MAX,
											 ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
											 0,  NULL,
											 BLE_APP_ADV_CHANNEL,
											 ADV_FP_NONE);
		}

	if(adv_param_status != BLE_SUCCESS){
		//tlkapi_printf(APP_LOG_EN, "[APP][INI] ADV parameters error 0x%x!!!\n", adv_param_status);
	}

	hal_set_ble_data();
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable

	/* set RF power index, user must set it after every suspend wake_up, because relative setting will be reset in suspend */
	rf_set_power_level_index (BLE_RF_POWER_INDEX);

	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &task_suspend_exit);
	bls_app_registerEventCallback (BLT_EV_FLAG_DATA_LENGTH_EXCHANGE, &task_dle_exchange);

	///////////////////// Power Management initialization///////////////////
	#if(BLE_APP_PM_ENABLE)
		blc_ll_initPowerManagement_module();

		#if (PM_DEEPSLEEP_RETENTION_ENABLE)
		    blc_app_setDeepsleepRetentionSramSize(); //select DEEPSLEEP_MODE_RET_SRAM_LOW16K or DEEPSLEEP_MODE_RET_SRAM_LOW32K
			bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
			blc_pm_setDeepsleepRetentionThreshold(95, 95);

			blc_pm_setDeepsleepRetentionEarlyWakeupTiming(270);

		#else
			bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
		#endif

		bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &task_sleep_enter);
	#else
		bls_pm_setSuspendMask (SUSPEND_DISABLE);
	#endif
	
////////////////////////////////////////////////////////////////////////////////////////////////

	/* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
	 * attention that code will stuck in "while(1)" if any error detected in initialization, user need find what error happens and then fix it */
	blc_app_checkControllerHostInitialization();

#if (BLE_USE_AD_PACK == 1)
	ble_scanning_init();
#endif

	ble_keep_alive();
}

/**
 * @brief		user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]	none
 * @return      none
 */
_attribute_ram_code_ void ble_init_deepRetn(void)
{
#if (PM_DEEPSLEEP_RETENTION_ENABLE)

	blc_app_loadCustomizedParameters_deepRetn();

	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (BLE_RF_POWER_INDEX);

	blc_ll_recoverDeepRetention();
	
	DBG_CHN0_HIGH;    //debug

	irq_enable();
	bls_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable
	ble_keep_alive();

#if (BLE_USE_AD_PACK == 1)
	ble_scanning_init();
#endif
#endif
}


void ble_set_sleep_tick(u32 tick)
{
    ble_sleep_tick = tick;
}

u32 ble_get_sleep_tick(void)
{
    return ble_sleep_tick;
}

void ble_set_ota_tick(void)
{
    ble_keep_alive();
    ble_sleep_tick = BLE_OTA_ENTER_DEEP_TIME;
}

void ble_set_power_on(void)
{
    ble_keep_alive();
    ble_sleep_tick = BLE_POWERON_ENTER_DEEP_TIME;
}

void ble_set_power_off(void)
{
    ble_keep_alive();
    ble_sleep_tick = BLE_POWEROFF_ENTER_DEEP_TIME;
}

void ble_keep_alive(void)
{
    latest_user_event_tick = clock_time();
}

u32 ble_get_alive_tick(void)
{
    return latest_user_event_tick;
}

void ble_init(int deepRetWakeUp)
{
    if(deepRetWakeUp)
    {
        ble_init_deepRetn();
    }else{
        ble_init_normal();
    }
    ble_set_power_on();
}
#endif
