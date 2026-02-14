#ifndef PHNETSTATE_H
#define PHNETSTATE_H

class NET_Packet;

struct SPHNetState
{
	float3 linear_vel;
	float3 angular_vel;
	float3 force;
	float3 torque;
	float3 position;
	float3 previous_position;
	union {
		Fquaternion quaternion;
		struct
		{
			float3 accel;
			float max_velocity;
		};
	};
	Fquaternion previous_quaternion;
	bool enabled;
	void net_Export(NET_Packet& P);
	void net_Import(NET_Packet& P);
	void net_Import(IReader& P);
	void net_Save(NET_Packet& P);
	void net_Load(NET_Packet& P);
	void net_Load(IReader& P);
	void net_Save(NET_Packet& P, const float3& min, const float3& max);
	void net_Load(NET_Packet& P, const float3& min, const float3& max);
	void net_Load(IReader& P, const float3& min, const float3& max);

  private:
	template <typename src> void read(src& P);
	template <typename src> void read(src& P, const float3& min, const float3& max);
};

DEFINE_VECTOR(SPHNetState, PHNETSTATE_VECTOR, PHNETSTATE_I);

struct SPHBonesData
{
	u64 bones_mask;
	u16 root_bone;
	PHNETSTATE_VECTOR bones;
	float3 m_min;
	float3 m_max;

  public:
	SPHBonesData();
	void net_Save(NET_Packet& P);
	void net_Load(NET_Packet& P);
	void net_Load(IReader& P);
	void set_min_max(const float3& _min, const float3& _max);
	const float3& get_min() const
	{
		return m_min;
	}
	const float3& get_max() const
	{
		return m_max;
	}
};
#endif
