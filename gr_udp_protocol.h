//
// Created by RGAA on 12/08/2026.
// GameStream-style UDP media protocol (custom, NOT wire-compatible with GameStream).
// Shared by render (net_udp plugin) and client (tc_client_sdk_new).
// See docs/udp_gamestream_channel_plan.md
//

#ifndef GAMMARAY_GR_UDP_PROTOCOL_H
#define GAMMARAY_GR_UDP_PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "data.h"

namespace tc
{

    // ---------------- wire format (little-endian) ----------------
    //
    // Common header (4B): magic(u16)='GU' | version(u8) | pkt_type(u8)
    //
    // Video shard (pkt_type=1), base header 20B:
    //   frame_index(u32) | timestamp_ms(u32) | flags(u8) | fec_block(u8) |
    //   data_shards(u16) | parity_shards(u16) | shard_index(u16) | payload_len(u16) |
    //   mon_slot(u8) | codec(u8)
    // SOF extension (only when flags & kFlagSof), right after base header:
    //   frame_width(u16) | frame_height(u16) | mon_name_len(u8) | mon_name bytes
    // then payload(payload_len bytes)
    //
    // Ctrl packet (pkt_type=3): subtype(u8) + body
    //   kCtrlHello(1):     device_id_len(u8)+device_id | stream_id_len(u8)+stream_id
    //   kCtrlHeartbeat(2): stream_id_len(u8)+stream_id
    //   kCtrlIdrRequest(3):mon_name_len(u8)+mon_name   (empty = all monitors)
    //   kCtrlFrameStatus(4): frame_index(u32) | received(u16) | lost(u16)
    //   kCtrlKick(5):      reason_len(u8)+reason   (render -> client, e.g. taken over)

    class GrUdpProtocol {
    public:
        static constexpr uint16_t kMagic = 0x4755; // 'GU'
        static constexpr uint8_t kVersion = 1;

        static constexpr uint8_t kPktVideo = 1;
        static constexpr uint8_t kPktAudio = 2;
        static constexpr uint8_t kPktCtrl = 3;

        static constexpr uint8_t kFlagKey = 0x1;
        static constexpr uint8_t kFlagSof = 0x2;
        static constexpr uint8_t kFlagEof = 0x4;

        static constexpr uint8_t kCodecH264 = 0;
        static constexpr uint8_t kCodecH265 = 1;

        static constexpr uint8_t kCtrlHello = 1;
        static constexpr uint8_t kCtrlHeartbeat = 2;
        static constexpr uint8_t kCtrlIdrRequest = 3;
        static constexpr uint8_t kCtrlFrameStatus = 4;
        static constexpr uint8_t kCtrlKick = 5;

        static constexpr int kCommonHeaderSize = 4;
        static constexpr int kVideoHeaderSize = 20;
        static constexpr int kDefaultMtu = 1400;
        static constexpr int kWanMtu = 1024;
        static constexpr int kMaxMonNameLen = 64;

        // ---- little-endian read/write helpers ----
        static void W16(char* p, uint16_t v) { p[0] = (char)(v & 0xff); p[1] = (char)((v >> 8) & 0xff); }
        static void W32(char* p, uint32_t v) { p[0] = (char)(v & 0xff); p[1] = (char)((v >> 8) & 0xff); p[2] = (char)((v >> 16) & 0xff); p[3] = (char)((v >> 24) & 0xff); }
        static uint16_t R16(const char* p) { return (uint16_t)((uint8_t)p[0] | ((uint8_t)p[1] << 8)); }
        static uint32_t R32(const char* p) { return (uint32_t)((uint8_t)p[0] | ((uint8_t)p[1] << 8) | ((uint8_t)p[2] << 16) | ((uint32_t)(uint8_t)p[3] << 24)); }

        static void WriteCommon(char* p, uint8_t pkt_type) {
            W16(p, kMagic);
            p[2] = (char)kVersion;
            p[3] = (char)pkt_type;
        }

        // returns pkt_type (>0) when valid, 0 otherwise
        static uint8_t ParseCommon(const char* data, size_t size) {
            if (!data || size < kCommonHeaderSize) return 0;
            if (R16(data) != kMagic) return 0;
            if ((uint8_t)data[2] != kVersion) return 0;
            uint8_t t = (uint8_t)data[3];
            if (t < kPktVideo || t > kPktCtrl) return 0;
            return t;
        }

        // ---- video shard ----
        struct VideoShardInfo {
            uint32_t frame_index_ = 0;
            uint32_t timestamp_ms_ = 0;
            uint8_t flags_ = 0;
            uint8_t fec_block_ = 0;
            uint16_t data_shards_ = 0;
            uint16_t parity_shards_ = 0;
            uint16_t shard_index_ = 0;
            uint16_t payload_len_ = 0;
            uint8_t mon_slot_ = 0;
            uint8_t codec_ = 0;
            // SOF extension
            uint16_t frame_width_ = 0;
            uint16_t frame_height_ = 0;
            std::string mon_name_;
            // payload view into the original packet
            const char* payload_ = nullptr;
        };

        // parse a full UDP packet (including common header) as a video shard
        static bool ParseVideoShard(const char* data, size_t size, VideoShardInfo& out) {
            if (ParseCommon(data, size) != kPktVideo) return false;
            if (size < kCommonHeaderSize + kVideoHeaderSize) return false;
            const char* p = data + kCommonHeaderSize;
            out.frame_index_ = R32(p);
            out.timestamp_ms_ = R32(p + 4);
            out.flags_ = (uint8_t)p[8];
            out.fec_block_ = (uint8_t)p[9];
            out.data_shards_ = R16(p + 10);
            out.parity_shards_ = R16(p + 12);
            out.shard_index_ = R16(p + 14);
            out.payload_len_ = R16(p + 16);
            out.mon_slot_ = (uint8_t)p[18];
            out.codec_ = (uint8_t)p[19];
            size_t off = kCommonHeaderSize + kVideoHeaderSize;
            if (out.flags_ & kFlagSof) {
                if (size < off + 5) return false;
                const char* e = data + off;
                out.frame_width_ = R16(e);
                out.frame_height_ = R16(e + 2);
                uint8_t nl = (uint8_t)e[4];
                if (nl > kMaxMonNameLen || size < off + 5 + nl) return false;
                out.mon_name_.assign(e + 5, nl);
                off += 5 + nl;
            }
            if (size != off + out.payload_len_) return false;
            out.payload_ = data + off;
            return true;
        }

        struct VideoFrameMeta {
            uint32_t frame_index_ = 0;
            uint32_t timestamp_ms_ = 0;
            bool key_ = false;
            uint8_t codec_ = kCodecH264;
            uint16_t frame_width_ = 0;
            uint16_t frame_height_ = 0;
            uint8_t mon_slot_ = 0;
            std::string mon_name_;
        };

        // split one encoded frame into UDP packets (<= mtu each)
        static std::vector<std::shared_ptr<Data>> ShardVideoFrame(const VideoFrameMeta& meta,
                                                                  const char* data, size_t size,
                                                                  int mtu = kDefaultMtu) {
            std::vector<std::shared_ptr<Data>> out;
            if (!data || size == 0 || meta.mon_name_.size() > kMaxMonNameLen) return out;
            const int sof_ext = 5 + (int)meta.mon_name_.size();
            const int base = kCommonHeaderSize + kVideoHeaderSize;
            const int first_payload = mtu - base - sof_ext;
            const int next_payload = mtu - base;
            if (first_payload <= 0) return out;
            size_t total = (size <= (size_t)first_payload)
                               ? 1
                               : 1 + (size - first_payload + next_payload - 1) / next_payload;
            if (total > 0xffff) return out; // frame way too large, give up
            out.reserve(total);

            size_t sent = 0;
            for (size_t i = 0; i < total; i++) {
                bool sof = (i == 0);
                bool eof = (i == total - 1);
                int payload_cap = sof ? first_payload : next_payload;
                int plen = (int)std::min<size_t>(payload_cap, size - sent);
                size_t pkt_size = base + (sof ? sof_ext : 0) + plen;
                auto buf = Data::Make(nullptr, pkt_size);
                char* p = buf->DataAddr();
                WriteCommon(p, kPktVideo);
                char* v = p + kCommonHeaderSize;
                W32(v, meta.frame_index_);
                W32(v + 4, meta.timestamp_ms_);
                uint8_t flags = (meta.key_ ? kFlagKey : 0) | (sof ? kFlagSof : 0) | (eof ? kFlagEof : 0);
                v[8] = (char)flags;
                v[9] = 0; // fec_block, reserved for P2
                W16(v + 10, (uint16_t)total);
                W16(v + 12, 0); // parity_shards, reserved for P2
                W16(v + 14, (uint16_t)i);
                W16(v + 16, (uint16_t)plen);
                v[18] = (char)meta.mon_slot_;
                v[19] = (char)meta.codec_;
                size_t off = base;
                if (sof) {
                    char* e = p + off;
                    W16(e, meta.frame_width_);
                    W16(e + 2, meta.frame_height_);
                    e[4] = (char)meta.mon_name_.size();
                    memcpy(e + 5, meta.mon_name_.data(), meta.mon_name_.size());
                    off += sof_ext;
                }
                memcpy(p + off, data + sent, plen);
                sent += plen;
                out.push_back(buf);
            }
            return out;
        }

        // ---- ctrl builders ----
        static std::shared_ptr<Data> BuildCtrlString2(uint8_t subtype, const std::string& a, const std::string& b) {
            size_t n = kCommonHeaderSize + 1 + 1 + a.size() + 1 + b.size();
            auto buf = Data::Make(nullptr, n);
            char* p = buf->DataAddr();
            WriteCommon(p, kPktCtrl);
            p[kCommonHeaderSize] = (char)subtype;
            char* q = p + kCommonHeaderSize + 1;
            *q++ = (char)a.size();
            memcpy(q, a.data(), a.size()); q += a.size();
            *q++ = (char)b.size();
            memcpy(q, b.data(), b.size());
            return buf;
        }

        static std::shared_ptr<Data> BuildCtrlString1(uint8_t subtype, const std::string& a) {
            size_t n = kCommonHeaderSize + 1 + 1 + a.size();
            auto buf = Data::Make(nullptr, n);
            char* p = buf->DataAddr();
            WriteCommon(p, kPktCtrl);
            p[kCommonHeaderSize] = (char)subtype;
            char* q = p + kCommonHeaderSize + 1;
            *q++ = (char)a.size();
            memcpy(q, a.data(), a.size());
            return buf;
        }

        static std::shared_ptr<Data> BuildHello(const std::string& device_id, const std::string& stream_id) {
            return BuildCtrlString2(kCtrlHello, device_id, stream_id);
        }
        static std::shared_ptr<Data> BuildHeartbeat(const std::string& stream_id) {
            return BuildCtrlString1(kCtrlHeartbeat, stream_id);
        }
        static std::shared_ptr<Data> BuildIdrRequest(const std::string& mon_name) {
            return BuildCtrlString1(kCtrlIdrRequest, mon_name);
        }
        static std::shared_ptr<Data> BuildKick(const std::string& reason) {
            return BuildCtrlString1(kCtrlKick, reason);
        }

        // parse ctrl packet body; returns subtype(>0) or 0.
        // strings are filled for Hello(device_id,stream_id) / Heartbeat(stream_id) /
        // IdrRequest(mon_name) / Kick(reason).
        static uint8_t ParseCtrl(const char* data, size_t size,
                                 std::string& s1, std::string& s2) {
            if (ParseCommon(data, size) != kPktCtrl) return 0;
            if (size < kCommonHeaderSize + 1) return 0;
            uint8_t subtype = (uint8_t)data[kCommonHeaderSize];
            const char* q = data + kCommonHeaderSize + 1;
            size_t left = size - kCommonHeaderSize - 1;
            auto read_str = [&](std::string& out) -> bool {
                if (left < 1) return false;
                uint8_t nl = (uint8_t)*q++;
                left--;
                if (left < nl) return false;
                out.assign(q, nl);
                q += nl;
                left -= nl;
                return true;
            };
            s1.clear(); s2.clear();
            switch (subtype) {
                case kCtrlHello:
                    if (!read_str(s1) || !read_str(s2)) return 0;
                    return subtype;
                case kCtrlHeartbeat:
                case kCtrlIdrRequest:
                case kCtrlKick:
                    if (!read_str(s1)) return 0;
                    return subtype;
                default:
                    return 0;
            }
        }
    };

    // ---------------- client-side frame reassembler ----------------
    //
    // Collects video shards (per mon_slot), emits complete frames.
    // Loss policy (P1, no FEC yet): a newer frame_index for the same mon_slot
    // declares the in-progress frame lost; after any loss, P frames are dropped
    // until a key frame completes (mirrors the webrtc_local convention that the
    // first delivered frame must be an IDR).
    class GrUdpFrameReassembler {
    public:
        struct CompleteFrame {
            uint8_t mon_slot_ = 0;
            std::string mon_name_;
            uint32_t frame_index_ = 0;
            uint32_t timestamp_ms_ = 0;
            bool key_ = false;
            uint8_t codec_ = 0;
            uint16_t frame_width_ = 0;
            uint16_t frame_height_ = 0;
            std::shared_ptr<Data> data_;
        };

        // completed, decodable frame
        std::function<void(const CompleteFrame&)> on_frame_;
        // a frame was declared lost (gap); client should request an IDR for this slot
        std::function<void(uint8_t mon_slot, uint32_t lost_frame_index)> on_frame_lost_;

        // feed one raw UDP packet (common header included)
        void AddPacket(const char* data, size_t size) {
            GrUdpProtocol::VideoShardInfo shard;
            if (!GrUdpProtocol::ParseVideoShard(data, size, shard)) return;
            if (shard.data_shards_ == 0 || shard.shard_index_ >= shard.data_shards_) return;

            auto& cur = assemblies_[shard.mon_slot_];
            if (cur.active_ && shard.frame_index_ > cur.frame_index_) {
                // newer frame arrived while current incomplete -> declare loss
                DeclareLoss(shard.mon_slot_, cur.frame_index_);
                cur = Assembly{};
            }
            if (cur.active_ && shard.frame_index_ < cur.frame_index_) {
                return; // stale shard of an already-lost/completed frame
            }
            if (!cur.active_) {
                if (!(shard.flags_ & GrUdpProtocol::kFlagSof)) {
                    // joining mid-frame: we cannot trust earlier shards, treat as broken
                    DeclareLoss(shard.mon_slot_, shard.frame_index_);
                    return;
                }
                cur.active_ = true;
                cur.frame_index_ = shard.frame_index_;
                cur.timestamp_ms_ = shard.timestamp_ms_;
                cur.key_ = (shard.flags_ & GrUdpProtocol::kFlagKey) != 0;
                cur.codec_ = shard.codec_;
                cur.frame_width_ = shard.frame_width_;
                cur.frame_height_ = shard.frame_height_;
                cur.mon_name_ = shard.mon_name_;
                cur.shards_.resize(shard.data_shards_);
                cur.received_ = 0;
            }
            // same frame
            if (cur.shards_.size() != shard.data_shards_) {
                // inconsistent shard count, frame is broken
                DeclareLoss(shard.mon_slot_, cur.frame_index_);
                cur = Assembly{};
                return;
            }
            auto& slot = cur.shards_[shard.shard_index_];
            if (!slot.filled_) {
                slot.filled_ = true;
                slot.bytes_.assign(shard.payload_, shard.payload_len_);
                cur.received_++;
            }
            if (shard.flags_ & GrUdpProtocol::kFlagSof) {
                cur.mon_name_ = shard.mon_name_;
                cur.frame_width_ = shard.frame_width_;
                cur.frame_height_ = shard.frame_height_;
            }
            if (cur.received_ == cur.shards_.size()) {
                CompleteFrame f;
                f.mon_slot_ = shard.mon_slot_;
                f.mon_name_ = cur.mon_name_;
                f.frame_index_ = cur.frame_index_;
                f.timestamp_ms_ = cur.timestamp_ms_;
                f.key_ = cur.key_;
                f.codec_ = cur.codec_;
                f.frame_width_ = cur.frame_width_;
                f.frame_height_ = cur.frame_height_;
                size_t total = 0;
                for (auto& s : cur.shards_) total += s.bytes_.size();
                f.data_ = Data::Make(nullptr, total);
                size_t off = 0;
                for (auto& s : cur.shards_) {
                    memcpy(f.data_->DataAddr() + off, s.bytes_.data(), s.bytes_.size());
                    off += s.bytes_.size();
                }
                bool decodable = f.key_ || !need_key_[shard.mon_slot_];
                if (f.key_) need_key_[shard.mon_slot_] = false;
                cur = Assembly{};
                if (decodable && on_frame_) on_frame_(f);
            }
        }

    private:
        void DeclareLoss(uint8_t mon_slot, uint32_t frame_index) {
            need_key_[mon_slot] = true;
            if (on_frame_lost_) on_frame_lost_(mon_slot, frame_index);
        }

        struct ShardSlot {
            bool filled_ = false;
            std::string bytes_;
        };
        struct Assembly {
            bool active_ = false;
            uint32_t frame_index_ = 0;
            uint32_t timestamp_ms_ = 0;
            bool key_ = false;
            uint8_t codec_ = 0;
            uint16_t frame_width_ = 0;
            uint16_t frame_height_ = 0;
            std::string mon_name_;
            std::vector<ShardSlot> shards_;
            size_t received_ = 0;
        };

        std::map<uint8_t, Assembly> assemblies_;
        std::map<uint8_t, bool> need_key_;
    };

}

#endif //GAMMARAY_GR_UDP_PROTOCOL_H
