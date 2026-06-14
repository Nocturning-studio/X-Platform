#ifndef PHNETSTATE_H
#define PHNETSTATE_H

class NET_Packet;

struct SPHNetState
{
	fvec3 linear_vel;
	fvec3 angular_vel;
	fvec3 force;
	fvec3 torque;
	fvec3 position;
	fvec3 previous_position;
	union {
		Fquaternion quaternion;
		struct
		{
			fvec3 accel;
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
	void net_Save(NET_Packet& P, const fvec3& min, const fvec3& max);
	void net_Load(NET_Packet& P, const fvec3& min, const fvec3& max);
	void net_Load(IReader& P, const fvec3& min, const fvec3& max);

  private:
	template <typename src> void read(src& P);
	template <typename src> void read(src& P, const fvec3& min, const fvec3& max);
};

DEFINE_VECTOR(SPHNetState, PHNETSTATE_VECTOR, PHNETSTATE_I);

struct SPHBonesData
{
	u64 bones_mask;
	u16 root_bone;
	PHNETSTATE_VECTOR bones;
	fvec3 m_min;
	fvec3 m_max;

  public:
	SPHBonesData();
	void net_Save(NET_Packet& P);
	void net_Load(NET_Packet& P);
	void net_Load(IReader& P);
	void set_min_max(const fvec3& _min, const fvec3& _max);
	const fvec3& get_min() const
	{
		return m_min;
	}
	const fvec3& get_max() const
	{
		return m_max;
	}
};
#endif
