//
// Created by RGAA on 12/08/2026.
// Unit tests for gr_udp_protocol.h (shard / reassemble / ctrl packets)
//

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>

#include "tc_common_new/gr_udp_protocol.h"

using namespace tc;

static std::string MakeFrameBytes(size_t size) {
    std::string s(size, '\0');
    for (size_t i = 0; i < size; i++) s[i] = (char)(i % 251);
    return s;
}

static GrUdpProtocol::VideoFrameMeta MakeMeta(uint32_t frame_index, bool key) {
    GrUdpProtocol::VideoFrameMeta meta;
    meta.frame_index_ = frame_index;
    meta.timestamp_ms_ = 123456;
    meta.key_ = key;
    meta.codec_ = GrUdpProtocol::kCodecH264;
    meta.frame_width_ = 1920;
    meta.frame_height_ = 1080;
    meta.mon_slot_ = 1;
    meta.mon_name_ = R"(\\.\DISPLAY1)";
    return meta;
}

TEST(GrUdpProtocol, CommonHeader) {
    char buf[4];
    GrUdpProtocol::WriteCommon(buf, GrUdpProtocol::kPktVideo);
    EXPECT_EQ(GrUdpProtocol::ParseCommon(buf, 4), GrUdpProtocol::kPktVideo);
    buf[0] = 0; // break magic
    EXPECT_EQ(GrUdpProtocol::ParseCommon(buf, 4), 0);
    EXPECT_EQ(GrUdpProtocol::ParseCommon(buf, 2), 0);
}

TEST(GrUdpProtocol, ShardSmallFrameSinglePacket) {
    auto frame = MakeFrameBytes(500);
    auto meta = MakeMeta(7, true);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_EQ(pkts.size(), 1u);

    GrUdpProtocol::VideoShardInfo shard;
    ASSERT_TRUE(GrUdpProtocol::ParseVideoShard(pkts[0]->CStr(), pkts[0]->Size(), shard));
    EXPECT_EQ(shard.frame_index_, 7u);
    EXPECT_EQ(shard.data_shards_, 1u);
    EXPECT_EQ(shard.shard_index_, 0u);
    EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagSof);
    EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagEof);
    EXPECT_TRUE(shard.flags_ & GrUdpProtocol::kFlagKey);
    EXPECT_EQ(shard.mon_name_, meta.mon_name_);
    EXPECT_EQ(shard.frame_width_, 1920);
    EXPECT_EQ(shard.codec_, GrUdpProtocol::kCodecH264);
    ASSERT_EQ(shard.payload_len_, 500u);
    EXPECT_EQ(std::memcmp(shard.payload_, frame.data(), 500), 0);
}

TEST(GrUdpProtocol, ShardLargeFrameMultiplePackets) {
    auto frame = MakeFrameBytes(10000);
    auto meta = MakeMeta(9, false);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 1u);
    for (size_t i = 0; i < pkts.size(); i++) {
        EXPECT_LE(pkts[i]->Size(), GrUdpProtocol::kDefaultMtu);
        GrUdpProtocol::VideoShardInfo shard;
        ASSERT_TRUE(GrUdpProtocol::ParseVideoShard(pkts[i]->CStr(), pkts[i]->Size(), shard));
        EXPECT_EQ(shard.data_shards_, pkts.size());
        EXPECT_EQ(shard.shard_index_, i);
        EXPECT_EQ(shard.flags_ & GrUdpProtocol::kFlagSof, i == 0 ? GrUdpProtocol::kFlagSof : 0);
        // mon_name only on SOF
        if (i == 0) EXPECT_EQ(shard.mon_name_, meta.mon_name_);
        else EXPECT_TRUE(shard.mon_name_.empty());
    }
}

TEST(GrUdpProtocol, ReassembleInOrder) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(3, true);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 1u);

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [](uint8_t, uint32_t) { FAIL() << "unexpected loss"; };

    for (auto& p : pkts) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_EQ(frames[0].frame_index_, 3u);
    EXPECT_TRUE(frames[0].key_);
    EXPECT_EQ(frames[0].mon_name_, meta.mon_name_);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, ReassembleOutOfOrder) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(4, false);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 2u);

    // P frame as the very first frame of the stream is decodable (no prior loss)
    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };

    // deliver: SOF first, then reverse the rest
    reasm.AddPacket(pkts[0]->CStr(), pkts[0]->Size());
    for (size_t i = pkts.size() - 1; i >= 1; i--) reasm.AddPacket(pkts[i]->CStr(), pkts[i]->Size());
    ASSERT_EQ(frames.size(), 1u);
    ASSERT_EQ(frames[0].data_->Size(), frame.size());
    EXPECT_EQ(std::memcmp(frames[0].data_->CStr(), frame.data(), frame.size()), 0);
}

TEST(GrUdpProtocol, LossDeclaresAndDropsPUntilKey) {
    auto frame_n = MakeFrameBytes(8000);
    auto meta_n = MakeMeta(10, false);
    auto pkts_n = GrUdpProtocol::ShardVideoFrame(meta_n, frame_n.data(), frame_n.size());
    ASSERT_GT(pkts_n.size(), 1u);

    auto frame_n1 = MakeFrameBytes(600);
    auto meta_n1 = MakeMeta(11, false);
    auto pkts_n1 = GrUdpProtocol::ShardVideoFrame(meta_n1, frame_n1.data(), frame_n1.size());

    auto frame_key = MakeFrameBytes(700);
    auto meta_key = MakeMeta(12, true);
    auto pkts_key = GrUdpProtocol::ShardVideoFrame(meta_key, frame_key.data(), frame_key.size());

    GrUdpFrameReassembler reasm;
    std::vector<GrUdpFrameReassembler::CompleteFrame> frames;
    std::vector<uint32_t> lost;
    reasm.on_frame_ = [&](const GrUdpFrameReassembler::CompleteFrame& f) { frames.push_back(f); };
    reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) { lost.push_back(idx); };

    // frame 10: deliver all but the last shard -> stuck incomplete
    for (size_t i = 0; i + 1 < pkts_n.size(); i++) reasm.AddPacket(pkts_n[i]->CStr(), pkts_n[i]->Size());
    EXPECT_TRUE(frames.empty());

    // frame 11 (newer) arrives -> frame 10 declared lost
    for (auto& p : pkts_n1) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], 10u);
    // frame 11 is a P frame after a loss -> completed but dropped
    EXPECT_TRUE(frames.empty());

    // key frame 12 -> delivered, stream recovered
    for (auto& p : pkts_key) reasm.AddPacket(p->CStr(), p->Size());
    ASSERT_EQ(frames.size(), 1u);
    EXPECT_TRUE(frames[0].key_);
    EXPECT_EQ(frames[0].frame_index_, 12u);
}

TEST(GrUdpProtocol, JoinMidFrameDeclaresLoss) {
    auto frame = MakeFrameBytes(8000);
    auto meta = MakeMeta(20, false);
    auto pkts = GrUdpProtocol::ShardVideoFrame(meta, frame.data(), frame.size());
    ASSERT_GT(pkts.size(), 1u);

    GrUdpFrameReassembler reasm;
    std::vector<uint32_t> lost;
    reasm.on_frame_lost_ = [&](uint8_t, uint32_t idx) { lost.push_back(idx); };
    reasm.on_frame_ = [](const GrUdpFrameReassembler::CompleteFrame&) { FAIL() << "should not complete"; };

    // first seen shard is NOT a SOF -> mid-frame join
    reasm.AddPacket(pkts[1]->CStr(), pkts[1]->Size());
    ASSERT_EQ(lost.size(), 1u);
    EXPECT_EQ(lost[0], 20u);
}

TEST(GrUdpProtocol, CtrlRoundtrip) {
    std::string s1, s2;

    auto hello = GrUdpProtocol::BuildHello("dev-123", "stream-abc");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(hello->CStr(), hello->Size(), s1, s2), GrUdpProtocol::kCtrlHello);
    EXPECT_EQ(s1, "dev-123");
    EXPECT_EQ(s2, "stream-abc");

    auto hb = GrUdpProtocol::BuildHeartbeat("stream-abc");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(hb->CStr(), hb->Size(), s1, s2), GrUdpProtocol::kCtrlHeartbeat);
    EXPECT_EQ(s1, "stream-abc");

    auto idr = GrUdpProtocol::BuildIdrRequest(R"(\\.\DISPLAY2)");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(idr->CStr(), idr->Size(), s1, s2), GrUdpProtocol::kCtrlIdrRequest);
    EXPECT_EQ(s1, R"(\\.\DISPLAY2)");

    auto kick = GrUdpProtocol::BuildKick("taken over");
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(kick->CStr(), kick->Size(), s1, s2), GrUdpProtocol::kCtrlKick);
    EXPECT_EQ(s1, "taken over");

    // truncated packet rejected
    ASSERT_EQ(GrUdpProtocol::ParseCtrl(hello->CStr(), hello->Size() - 3, s1, s2), 0);
}
