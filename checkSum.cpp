uint16_t
UdpHeader::CalculateHeaderChecksum (uint16_t size) const
{
  Buffer buf = Buffer ((2 * Address::MAX_SIZE) + 8);
  buf.AddAtStart ((2 * Address::MAX_SIZE) + 8);
  Buffer::Iterator it = buf.Begin ();
  uint32_t hdrSize = 0;

  WriteTo (it, m_source);
  WriteTo (it, m_destination);
  if (Ipv4Address::IsMatchingType (m_source))
    {
      it.WriteU8 (0); /* protocol */
      it.WriteU8 (m_protocol); /* protocol */
      it.WriteU8 (size >> 8); /* length */
      it.WriteU8 (size & 0xff); /* length */
      hdrSize = 12;
    }