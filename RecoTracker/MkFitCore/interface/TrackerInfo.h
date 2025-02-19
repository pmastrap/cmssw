#ifndef RecoTracker_MkFitCore_interface_TrackerInfo_h
#define RecoTracker_MkFitCore_interface_TrackerInfo_h

#include <string>
#include <vector>

namespace mkfit {

  class IterationsInfo;

  //==============================================================================

  enum WithinSensitiveRegion_e { WSR_Undef = -1, WSR_Inside = 0, WSR_Edge, WSR_Outside };

  struct WSR_Result {
    // Could also store XHitSize count equivalent here : 16;
    WithinSensitiveRegion_e m_wsr : 8;
    bool m_in_gap : 8;

    WSR_Result() : m_wsr(WSR_Undef), m_in_gap(false) {}

    WSR_Result(WithinSensitiveRegion_e wsr, bool in_gap) : m_wsr(wsr), m_in_gap(in_gap) {}
  };

  //==============================================================================

  class LayerInfo {
  public:
    enum LayerType_e { Undef = -1, Barrel = 0, EndCapPos = 1, EndCapNeg = 2 };

    LayerInfo(int lid, LayerType_e type) : m_layer_id(lid), m_layer_type(type) {}

    void set_layer_type(LayerType_e t) { m_layer_type = t; }
    void set_limits(float r1, float r2, float z1, float z2);
    void set_propagate_to(float pto) { m_propagate_to = pto; }
    void set_r_hole_range(float rh1, float rh2);
    void set_q_bin(float qb) { m_q_bin = qb; }
    void set_is_stereo(bool s) { m_is_stereo = s; }

    int layer_id() const { return m_layer_id; }
    LayerType_e layer_type() const { return m_layer_type; }
    float rin() const { return m_rin; }
    float rout() const { return m_rout; }
    float r_mean() const { return 0.5f * (m_rin + m_rout); }
    float zmin() const { return m_zmin; }
    float zmax() const { return m_zmax; }
    float z_mean() const { return 0.5f * (m_zmin + m_zmax); }
    float propagate_to() const { return m_propagate_to; }
    float q_bin() const { return m_q_bin; }
    bool is_stereo() const { return m_is_stereo; }

    bool is_barrel() const { return m_layer_type == Barrel; }

    bool is_within_z_limits(float z) const { return z > m_zmin && z < m_zmax; }
    bool is_within_r_limits(float r) const { return r > m_rin && r < m_rout; }
    bool is_within_q_limits(float q) const { return is_barrel() ? is_within_z_limits(q) : is_within_r_limits(q); }

    bool is_in_r_hole(float r) const { return m_has_r_range_hole ? is_in_r_hole_no_check(r) : false; }

    bool is_pixb_lyr() const { return m_is_pixb_lyr; }
    bool is_pixe_lyr() const { return m_is_pixe_lyr; }
    bool is_pix_lyr() const { return (m_is_pixb_lyr || m_is_pixe_lyr); }
    bool is_tib_lyr() const { return m_is_tib_lyr; }
    bool is_tob_lyr() const { return m_is_tob_lyr; }
    bool is_tid_lyr() const { return m_is_tid_lyr; }
    bool is_tec_lyr() const { return m_is_tec_lyr; }

    WSR_Result is_within_z_sensitive_region(float z, float dz) const {
      if (z > m_zmax + dz || z < m_zmin - dz)
        return WSR_Result(WSR_Outside, false);
      if (z < m_zmax - dz && z > m_zmin + dz)
        return WSR_Result(WSR_Inside, false);
      return WSR_Result(WSR_Edge, false);
    }

    WSR_Result is_within_r_sensitive_region(float r, float dr) const {
      if (r > m_rout + dr || r < m_rin - dr)
        return WSR_Result(WSR_Outside, false);
      if (r < m_rout - dr && r > m_rin + dr) {
        if (m_has_r_range_hole) {
          if (r < m_hole_r_max - dr && r > m_hole_r_min + dr)
            return WSR_Result(WSR_Outside, true);
          if (r < m_hole_r_max + dr && r > m_hole_r_min - dr)
            return WSR_Result(WSR_Edge, true);
        }
        return WSR_Result(WSR_Inside, false);
      }
      return WSR_Result(WSR_Edge, false);
    }

    void print_layer() const {
      printf("Layer %2d  r(%7.4f, %7.4f) z(% 9.4f, % 9.4f) is_brl=%d\n",
             m_layer_id,
             m_rin,
             m_rout,
             m_zmin,
             m_zmax,
             is_barrel());
    }

    // To be cleaned out with other geometry cleanup
    bool m_is_pixb_lyr = false;
    bool m_is_pixe_lyr = false;
    bool m_is_tib_lyr = false;
    bool m_is_tob_lyr = false;
    bool m_is_tid_lyr = false;
    bool m_is_tec_lyr = false;

  private:
    bool is_in_r_hole_no_check(float r) const { return r > m_hole_r_min && r < m_hole_r_max; }

    int m_layer_id = -1;
    LayerType_e m_layer_type = Undef;

    float m_rin, m_rout, m_zmin, m_zmax;
    float m_propagate_to;

    float m_q_bin;                     // > 0 - bin width, < 0 - number of bins
    float m_hole_r_min, m_hole_r_max;  // This could be turned into std::function when needed.
    bool m_has_r_range_hole = false;
    bool m_is_stereo = false;
  };

  //==============================================================================

  class TrackerInfo {
  public:
    enum EtaRegion {
      Reg_Begin = 0,
      Reg_Endcap_Neg = 0,
      Reg_Transition_Neg,
      Reg_Barrel,
      Reg_Transition_Pos,
      Reg_Endcap_Pos,
      Reg_End,
      Reg_Count = Reg_End
    };

    void reserve_layers(int n_brl, int n_ec_pos, int n_ec_neg);
    void create_layers(int n_brl, int n_ec_pos, int n_ec_neg);
    LayerInfo& new_barrel_layer();
    LayerInfo& new_ecap_pos_layer();
    LayerInfo& new_ecap_neg_layer();

    int n_layers() const { return m_layers.size(); }
    const LayerInfo& layer(int l) const { return m_layers[l]; }
    LayerInfo& layer_nc(int l) { return m_layers[l]; }

    const LayerInfo& operator[](int l) const { return m_layers[l]; }

    bool is_stereo(int i) const { return m_layers[i].is_stereo(); }
    bool is_pixb_lyr(int i) const { return m_layers[i].is_pixb_lyr(); }
    bool is_pixe_lyr(int i) const { return m_layers[i].is_pixe_lyr(); }
    bool is_pix_lyr(int i) const { return m_layers[i].is_pix_lyr(); }
    bool is_tib_lyr(int i) const { return m_layers[i].is_tib_lyr(); }
    bool is_tob_lyr(int i) const { return m_layers[i].is_tob_lyr(); }
    bool is_tid_lyr(int i) const { return m_layers[i].is_tid_lyr(); }
    bool is_tec_lyr(int i) const { return m_layers[i].is_tec_lyr(); }

    const LayerInfo& outer_barrel_layer() const { return m_layers[m_barrel.back()]; }

  private:
    int new_layer(LayerInfo::LayerType_e type);

    std::vector<LayerInfo> m_layers;

    std::vector<int> m_barrel;
    std::vector<int> m_ecap_pos;
    std::vector<int> m_ecap_neg;
  };

}  // end namespace mkfit
#endif
