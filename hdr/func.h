struct icmp_header
{
  u_int8_t type;		/* message type */
  u_int8_t code;		/* type sub-code */
  u_int16_t checksum;
  union
  {
    struct
    {
      u_int16_t	id;
      u_int16_t	sequence;
    } echo;					    /* echo datagram */
    u_int32_t	gateway;	    /* gateway address */
    struct
    {
      u_int16_t	__glibc_reserved;
      u_int16_t	mtu;
    } frag;		         	/* path mtu discovery */
  } un;
} __attribute__ ((__packed__));

struct ipv4_header
{
# if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
# elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
# else
# error "Please fix <bits/endian.h>"
# endif
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
    /*The options start here. */
} __attribute__ ((__packed__));

u_short checksum (u_short *hdr_pt, int hdr_len)
{
	long sum = 0;
	u_short tmp = 0;
	
	while (hdr_len > 1)
	{
		sum += *hdr_pt++;
		hdr_len -= sizeof (u_short);
	}
	if (hdr_len)
	{
		tmp = *(u_char *) hdr_pt << 8;
		sum += ntohs(tmp);
	}	
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (u_short)(~sum);
}
