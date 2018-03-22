#include <ros/ros.h>
#include <std_msgs/Int32.h>
#include <std_msgs/String.h>
#include <sensor_msgs/PointCloud2.h>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <rslidar_msgs/rslidarPic.h>
#include <rslidar_msgs/rslidarPacket.h>
#include <rslidar_msgs/rslidarScan.h>

ros::Publisher g_skippackets_num_pub1;
ros::Publisher g_skippackets_num_pub2;
ros::Publisher g_skippackets_num_pub3;
ros::Publisher g_maxnum_diff_packetnum_pub;
bool comparePair(const std::pair<unsigned, double> a, const std::pair<unsigned, double> b )
{
  return a.second < b.second;
}

void scanCallback(const rslidar_msgs::rslidarScan::ConstPtr &scan_msg1, const rslidar_msgs::rslidarScan::ConstPtr &scan_msg2, const rslidar_msgs::rslidarScan::ConstPtr &scan_msg3)
{
  // calculate the first packet timestamp difference (us)
  double time1 = scan_msg1->header.stamp.sec * 1e6 + scan_msg1->header.stamp.nsec * 0.001;
  double time2 = scan_msg2->header.stamp.sec * 1e6 + scan_msg2->header.stamp.nsec * 0.001;
  double time3 = scan_msg3->header.stamp.sec * 1e6 + scan_msg3->header.stamp.nsec * 0.001;

  std::vector<std::pair<unsigned, double> > lidar_vector;
  lidar_vector.push_back(std::make_pair(1, time1));
  lidar_vector.push_back(std::make_pair(2, time2));
  lidar_vector.push_back(std::make_pair(3, time3));

  std::sort(lidar_vector.begin(), lidar_vector.end(), comparePair);

  unsigned skip_big   = (lidar_vector[2].second - lidar_vector[0].second) / 1200;
  unsigned skip_small = (lidar_vector[1].second - lidar_vector[0].second) / 1200;

  std_msgs::String msg;
  std::stringstream ss;
  ss << "sync diff packets: " << skip_big;
  msg.data = ss.str();
  g_maxnum_diff_packetnum_pub.publish(msg);

  std_msgs::Int32 skip_num;
  if (skip_big > 1 && skip_small > 0)
  {
    // oldest skip 2 packets
    skip_num.data = 2;
    if (lidar_vector[0].first  == 1)
    {
      g_skippackets_num_pub1.publish(skip_num);
    }
    else if(lidar_vector[0].first == 2)
    {
      g_skippackets_num_pub2.publish(skip_num);
    }
    else if(lidar_vector[0].first == 3)
    {
      g_skippackets_num_pub3.publish(skip_num);
    }

    // middle skip 1 packets
    skip_num.data = 1;
    if (lidar_vector[1].first  == 1)
    {
      g_skippackets_num_pub1.publish(skip_num);
    }
    else if(lidar_vector[1].first == 2)
    {
      g_skippackets_num_pub2.publish(skip_num);
    }
    else if(lidar_vector[1].first == 3)
    {
      g_skippackets_num_pub3.publish(skip_num);
    }
  }
  else if ( skip_big > 0 && skip_small == 0)
  {
    // oldest skip 2 packets
    skip_num.data = 1;
    if (lidar_vector[0].first  == 1)
    {
      g_skippackets_num_pub1.publish(skip_num);
    }
    else if(lidar_vector[0].first == 2)
    {
      g_skippackets_num_pub2.publish(skip_num);
    }
    else if(lidar_vector[0].first == 3)
    {
      g_skippackets_num_pub3.publish(skip_num);
    }

    // middle skip 1 packets
    skip_num.data = 1;
    if (lidar_vector[1].first  == 1)
    {
      g_skippackets_num_pub1.publish(skip_num);
    }
    else if(lidar_vector[1].first == 2)
    {
      g_skippackets_num_pub2.publish(skip_num);
    }
    else if(lidar_vector[1].first == 3)
    {
      g_skippackets_num_pub3.publish(skip_num);
    }
  }
  else if ( skip_small > 0)
  {
    skip_num.data = 1;
    if (lidar_vector[0].first  == 1)
    {
      g_skippackets_num_pub1.publish(skip_num);
    }
    else if(lidar_vector[0].first == 2)
    {
      g_skippackets_num_pub2.publish(skip_num);
    }
    else if(lidar_vector[0].first == 3)
    {
      g_skippackets_num_pub3.publish(skip_num);
    }
    else
    {
      std::cerr << "Oldest lidar skip one packet!" << std::endl;
    }
  }
  if (skip_big == 0)
  {
    std::cout << "3 lidar synchronizer!" << std::endl;
  }
  // std::cout << std::endl;
}

int main(int argc, char **argv)
{
  ros::init(argc, argv, "multilidar_timesync");
  ros::NodeHandle nh("~");

  // get the topic parameters
  std::string scan1_topic;
  std::string scan2_topic;
  std::string scan3_topic;
  nh.getParam(std::string("scan1_topic"), scan1_topic);
  nh.getParam(std::string("scan2_topic"), scan2_topic);
  nh.getParam(std::string("scan3_topic"), scan3_topic);

  std::string skippackets1_topic;
  std::string skippackets2_topic;
  std::string skippackets3_topic;
  nh.getParam(std::string("skippackets1_topic"), skippackets1_topic);
  nh.getParam(std::string("skippackets2_topic"), skippackets2_topic);
  nh.getParam(std::string("skippackets3_topic"), skippackets3_topic);

  // sync the rslidarscan
  message_filters::Subscriber<rslidar_msgs::rslidarScan> scan_sub1(nh, scan1_topic.c_str(), 1);
  message_filters::Subscriber<rslidar_msgs::rslidarScan> scan_sub2(nh, scan2_topic.c_str(), 1);
  message_filters::Subscriber<rslidar_msgs::rslidarScan> scan_sub3(nh, scan3_topic.c_str(), 1);
  typedef message_filters::sync_policies::ApproximateTime<rslidar_msgs::rslidarScan, rslidar_msgs::rslidarScan, rslidar_msgs::rslidarScan> ScanSyncPolicy;
  message_filters::Synchronizer<ScanSyncPolicy> scan_sync(ScanSyncPolicy(10), scan_sub1, scan_sub2, scan_sub3);
  scan_sync.registerCallback(boost::bind(&scanCallback, _1, _2, _3));

  g_skippackets_num_pub1 = nh.advertise<std_msgs::Int32>(skippackets1_topic.c_str(), 1, true);
  g_skippackets_num_pub2 = nh.advertise<std_msgs::Int32>(skippackets2_topic.c_str(), 1, true);
  g_skippackets_num_pub3 = nh.advertise<std_msgs::Int32>(skippackets3_topic.c_str(), 1, true);

  std::string sync_packet_diff_topic("/sync_packet_diff");
  nh.getParam(std::string("sync_packet_diff_topic"), scan1_topic);
  g_maxnum_diff_packetnum_pub = nh.advertise<std_msgs::String>(sync_packet_diff_topic, 1, true);

  ros::spin();
  return 0;
}