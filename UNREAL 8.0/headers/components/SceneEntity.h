#pragma once

class SceneEntity { /* Every scene component inherits this class, it only contains relationship information between entities */
public:
	SceneEntity(int t_id) : m_id(t_id) {};

	int GetID() const { return m_id; };
	int GetHierachyValue() const { return m_hierachy_value; };

	void SetID(unsigned int id) { m_id = id; }
	void SetHierachyValue(unsigned int val) { m_hierachy_value = val; }
private:
	int m_id = -1;
	unsigned int m_hierachy_value = 0;
};